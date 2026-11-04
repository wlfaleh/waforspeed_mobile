// WaForSpeed — BinaryPack (WFS-89, épic E3) : format des messages de PeerJS 1.5.4 (serialization: 'binary').
// Port exact de peerjs-js-binarypack 2.1.0 (pack / unpack) : mêmes octets que le jeu web, pour qu'un téléphone et un
// navigateur échangent leurs présences dans le même salon. Types gérés : nul, booléen, nombre (entier ou double, comme
// le JavaScript : un nombre entier est écrit en entier), texte UTF-8, tableau, objet (clés texte, ordre gardé).
// Les données reçues ne sont jamais crues : unpack refuse (faux) un message tronqué, trop profond ou trop gros.
#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace wfs::bp {

struct Valeur {
    enum Type { Nul, Booleen, Nombre, Texte, Tableau, Objet } type = Nul;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Valeur> tab;
    std::vector<std::pair<std::string, Valeur>> obj;   // ordre d'insertion (Object.keys)

    static Valeur nul() { return {}; }
    static Valeur booleen(bool v) { Valeur x; x.type = Booleen; x.b = v; return x; }
    static Valeur nombre(double v) { Valeur x; x.type = Nombre; x.n = v; return x; }
    static Valeur texte(std::string v) { Valeur x; x.type = Texte; x.s = std::move(v); return x; }
    static Valeur tableau() { Valeur x; x.type = Tableau; return x; }
    static Valeur objet() { Valeur x; x.type = Objet; return x; }
    Valeur& ajoute(const std::string& cle, Valeur v) { obj.emplace_back(cle, std::move(v)); return obj.back().second; }
    const Valeur* champ(const std::string& cle) const {   // dernier champ de ce nom (map[key] = … du JavaScript)
        for (auto it = obj.rbegin(); it != obj.rend(); ++it) if (it->first == cle) return &it->second;
        return nullptr;
    }
};

std::vector<uint8_t> pack(const Valeur& v);
bool unpack(const uint8_t* data, size_t n, Valeur& out);
inline bool unpack(const std::vector<uint8_t>& d, Valeur& out) { return unpack(d.data(), d.size(), out); }

} // namespace wfs::bp
