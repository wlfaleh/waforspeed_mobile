// WaForSpeed — point d'entrée iOS (WFS-22) : application UIKit, même jeu que sur Android (Game, Renderer, systèmes).
// Rendu : Vulkan par MoltenVK (Metal). La surface Metal (VK_EXT_metal_surface) se crée dans VkCtx (src/render/vk.cpp),
// hors de la zone de ce ticket : en attendant ce branchement, le jeu tourne en rendu hors écran (comme le banc d'essai)
// et l'image est recopiée dans la vue deux fois par seconde. Paysage verrouillé, zone de sécurité (encoche, indicateur
// d'accueil) donnée par safeAreaInsets, tactile multi-doigts comme Android. Captures : Documents/ios-*.png (CI, ios.yml).
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include <fstream>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "../render/vk.h"

namespace wfs {
bool readAsset(const std::string& path, std::vector<uint8_t>& out) {
    const std::string racine = std::string([[[NSBundle mainBundle] resourcePath] UTF8String]) + "/assets/";
    std::ifstream f(racine + path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
double nowSeconds() { return CACurrentMediaTime(); }
}

using namespace wfs;

static const uint32_t LARGEUR = 1280, HAUTEUR = 720;   // rendu hors écran (paysage), mis à l'échelle de la vue
static VkCtx g_vk;
static Renderer g_rendu;
static Game g_jeu;
static bool g_pret = false;

static std::string dossierDocuments() {
    NSArray* d = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return std::string([d.firstObject UTF8String]);
}

@interface VueJeu : UIViewController
@end

@implementation VueJeu {
    CADisplayLink* _lien;
    UIImageView* _image;
    double _t0, _dernier;
    int _images;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:7 / 255.0 green:7 / 255.0 blue:12 / 255.0 alpha:1];
    self.view.multipleTouchEnabled = YES;
    _image = [[UIImageView alloc] initWithFrame:self.view.bounds];
    _image.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _image.contentMode = UIViewContentModeScaleToFill;
    [self.view addSubview:_image];
    jobs().start();
    if (g_vk.init(nullptr, LARGEUR, HAUTEUR) && g_rendu.init(g_vk) && g_jeu.init(g_rendu, "golf", "ville")) {
        g_jeu.allerA(Ecran::Menu);
        g_pret = true;
        WFS_LOG("QA pret : moteur initialise (iOS, MoltenVK, %ux%u hors écran)", LARGEUR, HAUTEUR);
    } else WFS_ERR("iOS : initialisation du moteur impossible (Vulkan / MoltenVK)");
    _t0 = _dernier = CACurrentMediaTime();
    _lien = [CADisplayLink displayLinkWithTarget:self selector:@selector(image:)];
    [_lien addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)viewSafeAreaInsetsDidChange {
    [super viewSafeAreaInsetsDidChange];
    const UIEdgeInsets m = self.view.safeAreaInsets;
    const CGFloat k = LARGEUR / std::max<CGFloat>(1, self.view.bounds.size.width);
    g_jeu.setMarges({(float)(m.left * k), (float)(m.top * k), (float)(m.right * k), (float)(m.bottom * k)});   // WFS-51
}

- (void)image:(CADisplayLink*)lien {
    if (!g_pret) return;
    const double t = CACurrentMediaTime();
    g_jeu.update(t - _dernier);
    _dernier = t;
    g_jeu.render(t - _t0);
    // recopie de l'image dans la vue (2 fois par seconde) et captures pour la CI (après 3 s et 10 s)
    if (++_images % 30 == 0) {
        const std::string png = dossierDocuments() + "/ios-apercu.png";
        if (g_rendu.capturePng(png)) _image.image = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:png.c_str()]];
    }
    if (_images == 180) g_rendu.capturePng(dossierDocuments() + "/ios-menu.png");
    if (_images == 200) { g_jeu.allerA(Ecran::Course); g_jeu.setAutopilot(true); }
    if (_images == 600) { g_rendu.capturePng(dossierDocuments() + "/ios-course.png"); WFS_LOG("QA iOS : captures écrites"); }
}

// tactile multi-doigts : coordonnées de la vue → pixels du rendu (paysage)
- (void)envoyer:(NSSet<UITouch*>*)touches appui:(bool)appui {
    const CGSize s = self.view.bounds.size;
    for (UITouch* t in touches) {
        const CGPoint p = [t locationInView:self.view];
        g_jeu.onTouch({(int)(((uintptr_t)(__bridge void*)t) & 0x7fffffff), (float)(p.x / s.width * LARGEUR), (float)(p.y / s.height * HAUTEUR), appui});
    }
}
- (void)touchesBegan:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self envoyer:t appui:true]; }
- (void)touchesMoved:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self envoyer:t appui:true]; }
- (void)touchesEnded:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self envoyer:t appui:false]; }
- (void)touchesCancelled:(NSSet<UITouch*>*)t withEvent:(UIEvent*)e { [self envoyer:t appui:false]; }

- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
@end

@interface Appli : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation Appli
- (BOOL)application:(UIApplication*)a didFinishLaunchingWithOptions:(NSDictionary*)o {
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.rootViewController = [[VueJeu alloc] init];
    [self.window makeKeyAndVisible];
    return YES;
}
// arrière-plan en course → pause (comme Android, WFS-39 RG-04)
- (void)applicationWillResignActive:(UIApplication*)a { if (g_pret && g_jeu.ecran() == Ecran::Course) g_jeu.allerA(Ecran::Pause); }
@end

int main(int argc, char* argv[]) {
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass([Appli class])); }
}
