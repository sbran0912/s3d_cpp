#pragma once

#include <string>

#include "mesh.hpp"

// Importiert werden nur die tatsaechlich verwendeten std-Namen.
using std::string;

// ====================================================================
// MESH-LADEN (Wavefront OBJ)
//
// Nur das Laden externer Geometrie. Es gibt bewusst KEINE Bau-/Speicher-
// Funktionen in der Bibliothek - Meshes werden ausserhalb erzeugt (Editor
// oder eigenes Skript) und hier eingelesen.
// ====================================================================

// LoadOBJ laedt ein Wavefront-OBJ-Mesh von der Festplatte und wandelt es in
// die flache Mesh-Darstellung der Bibliothek um:
//
//   - "v x y z"        -> Vertex-Pool
//   - "f ..."          -> Triangles (Dreiecke 1:1, Polygone per Fan-Triangulierung)
//   - "l ..."          -> Lines (aufeinanderfolgende Paare)
//
// Kommentare (#) und unbekannte Direktiven (vn, vt, g, o, s, mtllib, usemtl,
// ...) werden ignoriert. Indizes duerfen 1-basiert und negativ sein; die
// a/b/c-Schreibweise (Vertex/UV/Normal) wird akzeptiert, UV/Normal-Teile
// bleiben unberuecksichtigt.
//
// Die Koordinaten bleiben unveraendert (OBJ ist typischerweise Y-up, was zu
// "+Y zeigt nach oben" passt). Die gelieferten Dreiecke koennen direkt fuer
// bodies.MakeBody (Faces/Kollision) verwendet werden.
//
// Wirft runtime_error, wenn die Datei nicht gelesen oder das OBJ nicht
// gultig ist.
Mesh LoadOBJ(const string& path);
