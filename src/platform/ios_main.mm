// WaForSpeed — point d'entrée iOS (WFS-22, WFS-78) : application UIKit, même jeu que sur Android (Game, Renderer, systèmes).
// Rendu : Vulkan par MoltenVK (Metal), affiché directement dans le CAMetalLayer de la vue (VK_EXT_metal_surface, vk.cpp),
// à la cadence de l'écran (CADisplayLink). Initialisation répartie sur plusieurs images (Vulkan, rendu, puis jeu avec
// l'écran de chargement de WFS-35), chaque étape horodatée au journal. Paysage verrouillé, zone de sécurité donnée par
// safeAreaInsets, tactile multi-doigts comme Android, tout en pixels du calque. Captures CI : ios.yml (simctl io screenshot)
// sur les repères du journal « QA iOS : menu affiche » et « QA iOS : course affichee ».
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/chargement.h"

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

static VkCtx g_vk;
static Renderer g_rendu;
static Game g_jeu;
static bool g_pret = false;

// vue adossée à un CAMetalLayer : la swapchain Vulkan présente directement dedans (RG-01)
@interface VueMetal : UIView
@end
@implementation VueMetal
+ (Class)layerClass { return [CAMetalLayer class]; }
@end

@interface VueJeu : UIViewController
@end

@implementation VueJeu {
    CADisplayLink* _lien;
    double _t0, _dernier, _tPret, _tEtape;
    int _etape, _images;
    bool _menuVu, _courseLancee, _courseVue;
}

- (void)loadView {
    VueMetal* v = [[VueMetal alloc] initWithFrame:[UIScreen mainScreen].bounds];
    v.contentScaleFactor = [UIScreen mainScreen].nativeScale;   // calque en pixels réels de l'écran (RG-02)
    v.backgroundColor = [UIColor colorWithRed:7 / 255.0 green:7 / 255.0 blue:12 / 255.0 alpha:1];
    v.multipleTouchEnabled = YES;
    ((CAMetalLayer*)v.layer).opaque = YES;
    self.view = v;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    jobs().start();
    _t0 = _dernier = _tEtape = CACurrentMediaTime();
    _lien = [CADisplayLink displayLinkWithTarget:self selector:@selector(image:)];
    [_lien addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (CGFloat)echelle { return self.view.contentScaleFactor; }
- (uint32_t)largeurPx { return (uint32_t)(self.view.bounds.size.width * [self echelle]); }
- (uint32_t)hauteurPx { return (uint32_t)(self.view.bounds.size.height * [self echelle]); }

- (void)finEtape:(const char*)nom {
    const double t = CACurrentMediaTime();
    WFS_LOG("iOS : étape %s %.0f ms", nom, (t - _tEtape) * 1000.0);
    _tEtape = t;
}

// initialisation en plusieurs images : le fil principal rend la main à UIKit entre les étapes (RG-04)
- (void)initialiser {
    switch (_etape) {
        case 0:   // attendre que la vue soit dans la fenêtre, à sa taille de paysage
            if (!self.view.window || [self largeurPx] == 0) return;
            if ([self largeurPx] < [self hauteurPx] && CACurrentMediaTime() - _t0 < 3) return;   // rotation en paysage en cours
            [self finEtape:"vue"];
            break;
        case 1: {
            void* calque = (__bridge void*)self.view.layer;
            if (!g_vk.init(static_cast<ANativeWindow*>(calque))) { WFS_ERR("iOS : initialisation Vulkan impossible (MoltenVK)"); _etape = 99; return; }
            WFS_LOG("iOS : surface Metal créée, swapchain %ux%u (vue %ux%u px)", g_vk.swapExtent.width, g_vk.swapExtent.height,
                    [self largeurPx], [self hauteurPx]);
            [self finEtape:"vulkan"];
            break;
        }
        case 2:
            if (!g_rendu.init(g_vk)) { WFS_ERR("iOS : initialisation du rendu impossible"); _etape = 99; return; }
            g_rendu.setViewSize([self largeurPx], [self hauteurPx]);
            ecrans::chargement().debut(g_rendu, CACurrentMediaTime());   // WFS-35 : écran de chargement
            [self finEtape:"rendu"];
            break;
        case 3:
            if (!g_jeu.init(g_rendu, "golf", "ville", ecrans::Chargement::progression)) { ecrans::chargement().erreur(); _etape = 99; return; }
            ecrans::chargement().fin();
            g_jeu.allerA(Ecran::Menu);
            Game::demandes().debutChargement = [] { ecrans::chargement().debut(g_rendu, CACurrentMediaTime()); };   // WFS-74
            Game::demandes().progression = ecrans::Chargement::progression;
            Game::demandes().finChargement = [](bool ok) { if (ok) ecrans::chargement().fin(); else ecrans::chargement().erreur(); };
            [self viewSafeAreaInsetsDidChange];
            [self finEtape:"jeu"];
            g_pret = true;
            _dernier = _tPret = CACurrentMediaTime();
            WFS_LOG("QA pret : moteur initialise (iOS, MoltenVK, surface Metal %ux%u) en %.0f ms", g_vk.swapExtent.width,
                    g_vk.swapExtent.height, (_tPret - _t0) * 1000.0);
            break;
        default:
            if (ecrans::chargement().enErreur()) ecrans::chargement().dessiner();
            return;
    }
    _etape++;
}

- (void)viewSafeAreaInsetsDidChange {
    [super viewSafeAreaInsetsDidChange];
    if (!g_pret && _etape < 4) return;
    const UIEdgeInsets m = self.view.safeAreaInsets;
    const CGFloat k = [self echelle];
    g_jeu.setMarges({(float)(m.left * k), (float)(m.top * k), (float)(m.right * k), (float)(m.bottom * k)});   // WFS-51
}

- (void)image:(CADisplayLink*)lien {
    if (!g_pret) { [self initialiser]; return; }
    // rotation / changement de taille du calque → nouvelle swapchain (RG-02)
    if (++_images % 15 == 0 && g_vk.surfaceChanged()) {
        g_vk.waitIdle();
        if (g_vk.createSwapchain()) g_rendu.resize();
        WFS_LOG("Écran : %ux%u", g_vk.swapExtent.width, g_vk.swapExtent.height);
    }
    g_rendu.setViewSize([self largeurPx], [self hauteurPx]);
    const double t = CACurrentMediaTime();
    if (ecrans::chargement().enErreur()) ecrans::chargement().dessiner();
    else { g_jeu.update(t - _dernier); g_jeu.render(t - _t0); }
    _dernier = t;
    // repères pour les captures de la CI (ios.yml les prend avec simctl io screenshot)
    const double d = t - _tPret;
    if (!_menuVu && d > 6) { _menuVu = true; WFS_LOG("QA iOS : menu affiche (%d images)", _images); }
    if (!_courseLancee && d > 12) { _courseLancee = true; g_jeu.allerA(Ecran::Course); g_jeu.setAutopilot(true); }
    if (!_courseVue && d > 30) { _courseVue = true; WFS_LOG("QA iOS : course affichee (%d images, %.1f ips)", _images, _images / d); }
}

// tactile multi-doigts : points de la vue → pixels du calque (repère du rendu, RG-02)
- (void)envoyer:(NSSet<UITouch*>*)touches appui:(bool)appui {
    if (!g_pret) return;
    const CGFloat k = [self echelle];
    for (UITouch* t in touches) {
        const CGPoint p = [t locationInView:self.view];
        g_jeu.onTouch({(int)(t.hash & 0x7fffffff), (float)(p.x * k), (float)(p.y * k), appui});
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
