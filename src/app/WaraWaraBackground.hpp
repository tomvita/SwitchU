#pragma once
#include "../ui/Background.hpp"
#include "../core/Types.hpp"
#include <vector>

namespace ui {

class WaraWaraBackground : public Background {
public:
    WaraWaraBackground();

    void regenerate(int count = 50) override;

protected:
    void onUpdate(float dt) override;
    void onRender(Renderer& ren) override;

private:
    enum ShapeType { Circle, Triangle, Square, Diamond, Hexagon, ShapeCount };

    struct Shape {
        ShapeType type;
        Vec2  pos;
        float size;
        float speed;
        float phase;
        float wobble;
        float rotation;
        float rotSpeed;
        Color color;
        float glassAlpha;
    };

    void drawGlassShape(Renderer& ren, const Shape& s) const;
    void drawRoundedShape(Renderer& ren, const Shape& s, const Color& c) const;

    std::vector<Shape> m_shapes;
    float m_time = 0.f;
};

} // namespace ui
