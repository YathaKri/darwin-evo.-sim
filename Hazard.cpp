#include "Hazard.h"
#include <cmath>
#include <algorithm>

Hazard::Hazard(Vector2 pos, float radius, HazardType type, int lifetime)
    : m_position(pos), m_radius(radius), m_type(type), m_lifetime(lifetime)
{
    if (type == HazardType::Radiation) {
        // RAD: high permanent HP damage per frame, no lingering effect
        m_instantDamage = 1.5f;
        m_dotDamage     = 0.f;
        m_dotDuration   = 0;
    } else {
        // TOX: lower instant damage, but applies a lingering DOT
        m_instantDamage = 0.4f;
        m_dotDamage     = 0.3f;      // damage per frame while poisoned
        m_dotDuration   = 180;       // 3 seconds of lingering poison at 60fps
    }
}

void Hazard::update() {
    if (m_lifetime > 0) --m_lifetime;
}

void Hazard::draw() const {
    Color baseColor;
    if (m_type == HazardType::Radiation)
        baseColor = {255, 60, 60, 255};
    else
        baseColor = {160, 40, 255, 255};

    // Pulsing glow based on lifetime
    float pulse = 0.5f + 0.5f * std::sin((float)m_lifetime * 0.05f);
    unsigned char alpha = (unsigned char)(20 + 15 * pulse);

    // Layered glow rings
    DrawCircleV(m_position, m_radius,
                {baseColor.r, baseColor.g, baseColor.b, (unsigned char)(alpha / 2)});
    DrawCircleV(m_position, m_radius * 0.7f,
                {baseColor.r, baseColor.g, baseColor.b, alpha});
    DrawCircleV(m_position, m_radius * 0.4f,
                {baseColor.r, baseColor.g, baseColor.b, (unsigned char)std::min(255.f, alpha * 1.5f)});

    // Border ring
    DrawCircleLines((int)m_position.x, (int)m_position.y, m_radius,
                    {baseColor.r, baseColor.g, baseColor.b, (unsigned char)(40 + 20 * pulse)});

    // Label with type info
    const char* label = (m_type == HazardType::Radiation) ? "RAD" : "TOX";
    int textW = MeasureText(label, 10);
    DrawText(label, (int)m_position.x - textW / 2, (int)m_position.y - 5, 10,
             {baseColor.r, baseColor.g, baseColor.b, 120});

    // Sub-label for effect
    const char* effect = (m_type == HazardType::Radiation) ? "HP DMG" : "POISON";
    int effectW = MeasureText(effect, 7);
    DrawText(effect, (int)m_position.x - effectW / 2, (int)m_position.y + 6, 7,
             {baseColor.r, baseColor.g, baseColor.b, 70});
}

Hazard Hazard::spawn(std::mt19937& gen, float worldW, float worldH) {
    std::uniform_real_distribution<float> posX(80.f, worldW - 80.f);
    std::uniform_real_distribution<float> posY(80.f, worldH - 80.f);
    std::uniform_real_distribution<float> radDist(40.f, 80.f);
    std::uniform_int_distribution<int>    typeDist(0, 1);
    std::uniform_int_distribution<int>    lifeDist(300, 600);

    HazardType type = (typeDist(gen) == 0) ? HazardType::Radiation : HazardType::Toxic;
    return Hazard({posX(gen), posY(gen)}, radDist(gen), type, lifeDist(gen));
}
