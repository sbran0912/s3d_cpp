#include "vehicle.hpp"

#include <cmath>
#include <limits>

// Importiert werden nur die tatsaechlich verwendeten std-Namen.
using std::acos;
using std::atan2;
using std::size_t;
using std::sqrt;

// ====================================================================
// VEHICLE
// ====================================================================

Vehicle::Vehicle(const Body& b)
    : body{b},
      dna{
          RandomFloat(-1.0, 1.0),  // dna[0]: Kraft Richtung Gift
          RandomFloat(-1.0, 1.0),  // dna[1]: Kraft Richtung gutes Futter
          RandomFloat(20.0, 60.0), // dna[2]: Radius fuer Gift
          RandomFloat(20.0, 60.0), // dna[3]: Radius fuer gutes Futter
      } {}

void Vehicle::alignToVelocity() {
    const double mag = vel.Length();
    if (mag < 0.0001) {
        return;
    }

    const double magXZ = sqrt(vel.x * vel.x + vel.z * vel.z);

    // vel.y/mag muss in [-1,1] liegen (Schutz vor NaN durch Float-Rundung).
    const double cosY = Constrain(vel.y / mag, -1.0, 1.0);
    body.rotX = acos(cosY);
    if (magXZ < 0.0001) {
        body.rotY = 0.0;
    } else {
        body.rotY = atan2(vel.x / magXZ, vel.z / magXZ);
    }
    body.rotZ = 0.0;

    // Heading als normalisierte Richtung ableiten.
    heading = vel.Normalize();
}

void Vehicle::applyForce(Vec3 force) {
    accel = accel.Add(force);
}

void Vehicle::update() {
    vel = vel.Add(accel);
    const double speed = vel.Length();
    vel = vel.Normalize().Scale(Constrain(speed, 0.5, 2.0));
    accel = Vec3{};
    body.pos = body.pos.Add(vel);
}

// seekForce bestimmt, wie stark das Seek-Steering pro Frame in die
// Beschleunigung eingeht. Kleiner = ruhigeres, traegeres Anfliegen; die
// Geschwindigkeit selbst bleibt davon unberuehrt.
constexpr double seekForce = 0.1;

// steerLimit ist die Obergrenze der Steuerkraft (Laenge des Steering-Vektors).
// Kleiner = sanftere, weniger scharfe Korrekturen bei grossem Kursfehler,
// waehrend kleine Korrekturen unveraendert bleiben.
constexpr double steerLimit = 1.0;

void Vehicle::seek(Vec3 target, bool isBadFood) {
    Vec3 desired = target.Sub(body.pos).Limit(3.0);
    desired = isBadFood ? desired.Scale(dna[0]) : desired.Scale(dna[1]);
    const Vec3 steer = desired.Sub(vel).Limit(steerLimit);
    applyForce(steer.Scale(seekForce));
}

Food Vehicle::eatFood(Food food, bool isBadFood) {
    double filter = dna[3];
    if (isBadFood) {
        filter = dna[2];
    }
    double mindist = std::numeric_limits<double>::infinity();
    int idx = -1;

    for (size_t i = 0; i < food.size(); i++) {
        const double distance = body.pos.DistanceTo(food[i]);
        if (distance < filter && distance < mindist) {
            mindist = distance;
            idx = static_cast<int>(i);
        }
    }

    if (idx > -1) {
        if (body.pos.DistanceTo(food[static_cast<size_t>(idx)]) < 3.0) {
            // Food aufessen: aus dem Array entfernen genuegt – es gibt keinen
            // GPU-Buffer/Refcount mehr freizugeben (nur CPU-Geometrie).
            food.erase(food.begin() + idx);
            if (isBadFood) {
                health += -0.1;
            } else {
                health += 0.1;
            }
        } else {
            seek(food[static_cast<size_t>(idx)], isBadFood);
        }
    }
    return food;
}

void Vehicle::boundary() {
    constexpr double minX = -130.0;
    constexpr double maxX = 130.0;
    constexpr double minY = -130.0;
    constexpr double maxY = 130.0;
    constexpr double minZ = -130.0;
    constexpr double maxZ = 130.0;

    if (body.pos.x < minX || body.pos.x > maxX) {
        vel.x *= -1.0;
    }
    if (body.pos.y < minY || body.pos.y > maxY) {
        vel.y *= -1.0;
    }
    if (body.pos.z < minZ || body.pos.z > maxZ) {
        vel.z *= -1.0;
    }
}

bool Vehicle::isDead() const { return health < 0; }

// ====================================================================
// FUTTER
// ====================================================================

Food NewFood(int count) {
    Food food;
    food.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        food.push_back(RandomFoodPos());
    }
    return food;
}

Food Respawn(Food food, int min, int count) {
    if (static_cast<int>(food.size()) < min) {
        for (int i = 0; i < count; i++) {
            food.push_back(RandomFoodPos());
        }
    }
    return food;
}

// ====================================================================
// HILFSFUNKTIONEN
// ====================================================================

Mesh CenterMeshY(const Mesh& mesh, double dy) {
    Mesh out{mesh.Triangles, mesh.Lines};
    const float d = static_cast<float>(dy);
    for (size_t i = 1; i < out.Triangles.size(); i += 3) {
        out.Triangles[i] += d;
    }
    for (size_t i = 1; i < out.Lines.size(); i += 3) {
        out.Lines[i] += d;
    }
    return out;
}

Vec3 RandomFoodPos() {
    return Vec3{
        static_cast<double>(Random(-100, 100)),
        static_cast<double>(Random(-100, 100)),
        static_cast<double>(Random(-100, 100)),
    };
}
