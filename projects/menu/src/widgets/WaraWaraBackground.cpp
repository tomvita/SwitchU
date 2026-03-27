#include "WaraWaraBackground.hpp"
#include <nxui/core/Renderer.hpp>
#include <cmath>
#include <cstdlib>

WaraWaraBackground::WaraWaraBackground() { regenerate(30); }

void WaraWaraBackground::regenerate(int count) {
    m_shapes.resize(count);
    for (auto& s : m_shapes) {
        s.type       = static_cast<ShapeType>(std::rand() % ShapeCount);
        s.pos        = {(float)(std::rand() % 1280), (float)(std::rand() % 720)};
        s.size       = 14.f + (std::rand() % 40);
        s.speed      = 6.f  + (std::rand() % 22);
        s.phase      = (std::rand() % 1000) / 1000.f * 6.28f;
        s.wobble     = 10.f + (std::rand() % 18);
        s.rotation   = (std::rand() % 1000) / 1000.f * 6.28f;
        s.rotSpeed   = 0.12f + (std::rand() % 100) / 100.f * 0.5f;
        if (std::rand() % 2) s.rotSpeed = -s.rotSpeed;
        s.glassAlpha = 0.06f + (std::rand() % 100) / 100.f * 0.10f;
        float h = (std::rand() % 1000) / 1000.f;
        s.color = nxui::Color::fromHSL(h, 0.20f, 0.50f, s.glassAlpha);
    }
}

void WaraWaraBackground::onUpdate(float dt) {
    m_time += dt;
    for (auto& s : m_shapes) {
        s.pos.y -= s.speed * dt;
        s.pos.x += std::sin(m_time * 0.7f + s.phase) * s.wobble * dt;
        s.rotation += s.rotSpeed * dt;
        if (s.pos.y + s.size < -20.f) {
            s.pos.y = 740.f + s.size;
            s.pos.x = (float)(std::rand() % 1280);
        }
    }
}

void WaraWaraBackground::onRender(nxui::Renderer& ren) {
    ren.useShader(nxui::ShaderProgram::Gradient);
    nxui::FsUniforms fs = {};
    fs.useTexture = 0;
    fs.param1 = m_time;
    fs.extra[0] = m_accent.r;  fs.extra[1] = m_accent.g;
    fs.extra[2] = m_accent.b;  fs.extra[3] = m_accent.a;
    fs.extra[4] = m_secondary.r;  fs.extra[5] = m_secondary.g;
    fs.extra[6] = m_secondary.b;  fs.extra[7] = m_secondary.a;
    fs.extra[8]  = m_shapeColor.r * 2.f;  fs.extra[9]  = m_shapeColor.g * 2.f;
    fs.extra[10] = m_shapeColor.b * 2.f;  fs.extra[11] = m_shapeColor.a;
    ren.pushFsUniforms(fs);
    ren.drawRect(m_rect, nxui::Color::white());
    ren.flush();
    ren.useShader(nxui::ShaderProgram::Basic);

    for (auto& s : m_shapes)
        drawGlassShape(ren, s);
}

void WaraWaraBackground::drawGlassShape(nxui::Renderer& ren, const Shape& s) const {
    float a = s.color.a * m_opacity;
    if (a < 0.003f) return;

    nxui::Color body = m_shapeColor.withAlpha(a);
    drawRoundedShape(ren, s, body);

    Shape highlight = s;
    highlight.pos.y -= s.size * 0.08f;
    highlight.size   = s.size * 0.85f;
    nxui::Color hi = nxui::Color(1.f, 1.f, 1.f, a * 0.35f);
    drawRoundedShape(ren, highlight, hi);

    Shape edge = s;
    edge.size = s.size * 1.06f;
    nxui::Color edgeC = nxui::Color(1.f, 1.f, 1.f, a * 0.15f);
    drawRoundedShape(ren, edge, edgeC);
}

void WaraWaraBackground::drawRoundedShape(nxui::Renderer& ren, const Shape& s, const nxui::Color& c) const {
    float r  = s.rotation;
    float sz = s.size;

    auto rot = [&](float lx, float ly) -> nxui::Vec2 {
        float cs = std::cos(r), sn = std::sin(r);
        return {s.pos.x + lx * cs - ly * sn,
                s.pos.y + lx * sn + ly * cs};
    };

    switch (s.type) {
    case Circle:
        ren.drawCircle(s.pos, sz, c, 12);
        break;
    case Triangle: {
        nxui::Vec2 p0 = rot(0,             -sz);
        nxui::Vec2 p1 = rot(-sz * 0.866f,   sz * 0.5f);
        nxui::Vec2 p2 = rot( sz * 0.866f,   sz * 0.5f);
        ren.drawTriangle(p0, p1, p2, c);
        break;
    }
    case Square: {
        float h = sz * 0.707f;
        nxui::Vec2 p0 = rot(-h, -h);
        nxui::Vec2 p1 = rot( h, -h);
        nxui::Vec2 p2 = rot( h,  h);
        nxui::Vec2 p3 = rot(-h,  h);
        ren.drawTriangle(p0, p1, p2, c);
        ren.drawTriangle(p0, p2, p3, c);
        break;
    }
    case Diamond: {
        nxui::Vec2 p0 = rot(0,            -sz);
        nxui::Vec2 p1 = rot( sz * 0.6f,    0);
        nxui::Vec2 p2 = rot(0,             sz);
        nxui::Vec2 p3 = rot(-sz * 0.6f,    0);
        ren.drawTriangle(p0, p1, p2, c);
        ren.drawTriangle(p0, p2, p3, c);
        break;
    }
    case Hexagon: {
        constexpr int N = 6;
        const float step = 6.28318f / N;
        nxui::Vec2 pts[N];
        for (int i = 0; i < N; ++i) {
            float a2 = step * i;
            pts[i] = rot(std::cos(a2) * sz, std::sin(a2) * sz);
        }
        for (int i = 1; i < N - 1; ++i)
            ren.drawTriangle(pts[0], pts[i], pts[i + 1], c);
        break;
    }
    default: break;
    }
}

