#pragma once

#include <array>
#include <vector>

#include "bodies.hpp"

// Importiert werden nur die tatsaechlich verwendeten std-Namen.
using std::array;
using std::vector;

// ====================================================================
// FUTTER
//
// Food ist eine Menge von Futterpositionen. Das gemeinsame Mesh wird nicht
// hier verwaltet, daher reichen die Positionen.
// ====================================================================
using Food = vector<Vec3>;

// ====================================================================
// VEHICLE - Fahrzeug mit Steering-Behavior (Seek) und DNA
//
// Ein Vehicle ist ein Body (Mesh + Position/Rotation) mit zusaetzlichem
// Physik-Zustand (Geschwindigkeit/Beschleunigung), einer Gesundheit und einer
// DNA. Die DNA steuert, wie stark Gift bzw. gutes Futter angesteuert wird und
// in welchem Radius es ueberhaupt wahrgenommen wird.
//
// Definition: vehicle.cpp
// ====================================================================
struct Vehicle {
    Body body;
    Vec3 vel{};
    Vec3 accel{};
    Vec3 heading{};
    double health = 1.0;
    array<double, 4> dna{};

    // Vehicle erzeugt ein Fahrzeug aus einem Body mit zufaelliger DNA.
    Vehicle(const Body& b);

    // alignToVelocity passt die Richtung des Bodys an die Geschwindigkeit an.
    void alignToVelocity();

    // applyForce wendet eine Kraft an (akkumuliert in accel).
    void applyForce(Vec3 force);

    // update ist das Physik-Update: Geschwindigkeit aus Beschleunigung,
    // Position aus Geschwindigkeit. Die Geschwindigkeit wird auf [0.5, 2.0]
    // begrenzt.
    void update();

    // seek ist das Seek-Verhalten. Die gewuenschte Richtung wird je nach
    // Futter-Art mit dem passenden DNA-Wert skaliert (dna[0] fuer Gift, dna[1]
    // fuer gutes Futter).
    void seek(Vec3 target, bool isBadFood);

    // eatFood sucht das naechste Futter im DNA-Radius und frisst es, wenn es
    // nah genug ist. Gutes Futter -> Health +0.1, Gift -> Health -0.1.
    // Ansonsten wird es angesteuert (seek). Rueckgabe: ggf. gekuerztes
    // Food-Array.
    Food eatFood(Food food, bool isBadFood);

    // boundary kehrt die Geschwindigkeit an den Raendern um (Bounce).
    void boundary();

    // isDead gibt true zurueck, wenn das Vehicle tot ist (Health < 0).
    bool isDead() const;
};

// ====================================================================
// HILFSFUNKTIONEN
// ====================================================================

// NewFood erzeugt count Futterpositionen an Zufallspositionen.
Food NewFood(int count);

// Respawn fuellt das Futter-Array auf: sind weniger als min vorhanden, kommen
// count neue dazu.
Food Respawn(Food food, int min, int count);

// CenterMeshY verschiebt ein Mesh entlang Y. MakePyramidMesh legt die Basis
// bei y=0 (Spitze bei y=height); fuer ein um die Position zentriertes Fahrzeug
// wird das Mesh um -height/2 verschoben.
Mesh CenterMeshY(const Mesh& mesh, double dy);

// RandomFoodPos liefert eine zufaellige Futterposition in [-100, 100).
Vec3 RandomFoodPos();
