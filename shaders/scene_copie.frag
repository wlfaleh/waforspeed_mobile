#version 450
// WaForSpeed — WFS-425 : copie de la scène en MSAA ×1, 2e sous-passe de la passe de scène.
// La sous-passe 0 dessine la scène dans une couleur TEMPORAIRE (mémoire de tuile des GPU mobiles), comme le ×2 / ×4 ; cette
// sous-passe la recopie, pixel par pixel et dans la même tuile, dans la cible HDR relue par le bloom et la passe finale.
// R16G16B16A16 → flottant → R16G16B16A16 : copie exacte (image identique au bit près).
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput scene;
layout(location = 0) out vec4 couleur;
void main() { couleur = subpassLoad(scene); }
