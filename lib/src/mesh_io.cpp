#include "mesh_io.hpp"

#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "vec.hpp"

// Importiert werden nur die tatsaechlich verwendeten std-Namen.
using std::ifstream;
using std::istringstream;
using std::runtime_error;
using std::string;
using std::vector;

namespace {

// Trim entfernt vorangestellte und nachfolgende Whitespace-Zeichen.
string Trim(const string& s) {
    size_t b = 0;
    size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) {
        b++;
    }
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
        e--;
    }
    return s.substr(b, e - b);
}

// Fields zerlegt eine Zeile in Whitespace-getrennte Tokens.
vector<string> Fields(const string& line) {
    vector<string> out;
    istringstream iss(line);
    string tok;
    while (iss >> tok) {
        out.push_back(tok);
    }
    return out;
}

// ObjIndex parst ein Face-/Line-Token ("i", "i/j", "i/j/k", "i//k") zu einem
// 0-basierten Vertex-Index. vertexCount ist die Anzahl der bisher gelesenen
// Vertices (fuer negative Indizes und die Bereichspruefung). Wirft
// runtime_error bei ungultigem Token oder Index ausserhalb des Bereichs.
int ObjIndex(const string& tok, int vertexCount, int lineNo) {
    string s = tok;
    const size_t slash = s.find('/');
    if (slash != string::npos) {
        s = s.substr(0, slash); // UV/Normal-Teil abschneiden
    }
    if (s.empty()) {
        throw runtime_error("OBJ Zeile " + std::to_string(lineNo) + ": ungultiger Index \"" +
                            tok + "\"");
    }
    long n;
    try {
        n = std::stol(s);
    } catch (const std::exception&) {
        throw runtime_error("OBJ Zeile " + std::to_string(lineNo) + ": ungultiger Index \"" +
                            tok + "\"");
    }
    if (n < 0) {
        n = vertexCount + n; // negativ: relativ zum Ende
    } else {
        n = n - 1; // OBJ ist 1-basiert
    }
    if (n < 0 || n >= vertexCount) {
        throw runtime_error("OBJ Zeile " + std::to_string(lineNo) + ": Index \"" + tok +
                            "\" ausserhalb des Bereichs");
    }
    return static_cast<int>(n);
}

// ParseOBJ parst den OBJ-Text in ein Mesh.
Mesh ParseOBJ(const string& data) {
    vector<Vec3> verts;
    Mesh mesh;

    // Zeilen lesen (Zeilenumbrueche normalisieren: \r am Zeilenende entfernen,
    // d. h. Windows-CRLF wird behandelt).
    istringstream iss(data);
    string raw;
    int lineNo = 0;
    while (std::getline(iss, raw)) {
        lineNo++;
        if (!raw.empty() && raw.back() == '\r') {
            raw.pop_back();
        }
        const string line = Trim(raw);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const vector<string> fields = Fields(line);
        if (fields.empty()) {
            continue;
        }
        const int vertexCount = static_cast<int>(verts.size());

        if (fields[0] == "v") {
            if (fields.size() < 4) {
                throw runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                    ": unvollstaendiger Vertex");
            }
            double x, y, z;
            try {
                x = std::stod(fields[1]);
                y = std::stod(fields[2]);
                z = std::stod(fields[3]);
            } catch (const std::exception&) {
                throw runtime_error("OBJ Zeile " + std::to_string(lineNo) +
                                    ": ungueltiger Vertex");
            }
            verts.push_back(Vec3{x, y, z});

        } else if (fields[0] == "f") {
            vector<int> idx;
            idx.reserve(fields.size() - 1);
            for (size_t k = 1; k < fields.size(); k++) {
                idx.push_back(ObjIndex(fields[k], vertexCount, lineNo));
            }
            // Fan-Triangulierung: (0, j, j+1) fuer j = 1 .. n-2.
            for (size_t j = 1; j + 1 < idx.size(); j++) {
                const Vec3& a = verts[idx[0]];
                const Vec3& b = verts[idx[j]];
                const Vec3& c = verts[idx[j + 1]];
                mesh.Triangles.insert(
                    mesh.Triangles.end(),
                    {static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z),
                     static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z),
                     static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z)});
            }

        } else if (fields[0] == "l") {
            vector<int> idx;
            idx.reserve(fields.size() - 1);
            for (size_t k = 1; k < fields.size(); k++) {
                idx.push_back(ObjIndex(fields[k], vertexCount, lineNo));
            }
            // Aufeinanderfolgende Paare: (j, j+1).
            for (size_t j = 0; j + 1 < idx.size(); j++) {
                const Vec3& a = verts[idx[j]];
                const Vec3& b = verts[idx[j + 1]];
                mesh.Lines.insert(mesh.Lines.end(),
                                  {static_cast<float>(a.x), static_cast<float>(a.y),
                                   static_cast<float>(a.z), static_cast<float>(b.x),
                                   static_cast<float>(b.y), static_cast<float>(b.z)});
            }
        }
        // else: vn, vt, g, o, s, mtllib, usemtl, ... ignorieren.
    }

    return mesh;
}

} // namespace

Mesh LoadOBJ(const string& path) {
    ifstream file(path);
    if (!file) {
        throw runtime_error("OBJ-Datei nicht lesbar: " + path);
    }
    string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return ParseOBJ(data);
}
