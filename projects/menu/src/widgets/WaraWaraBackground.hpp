#pragma once
#include <nxui/widgets/Background.hpp>
#include <nxui/core/Types.hpp>
#include <vector>


class WaraWaraBackground : public nxui::Background {
public:
    WaraWaraBackground();

    void regenerate(int count = 50) override;

protected:
    void onUpdate(float dt) override;
    void onRender(nxui::Renderer& ren) override;

private:
    enum ShapeType { Circle, Triangle, Square, Diamond, Hexagon, ShapeCount };

    struct Shape {
        ShapeType type;
        nxui::Vec2  pos;
        float size;
        float speed;
        float phase;
        float wobble;
        float rotation;
        float rotSpeed;
        nxui::Color color;
        float glassAlpha;
    };

    void drawGlassShape(nxui::Renderer& ren, const Shape& s) const;
    void drawRoundedShape(nxui::Renderer& ren, const Shape& s, const nxui::Color& c) const;

    std::vector<Shape> m_shapes;
    float m_time = 0.f;
};

