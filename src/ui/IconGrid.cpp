#include "IconGrid.hpp"
#include "../app/GlossyIcon.hpp"
#include "../core/Renderer.hpp"
#include <algorithm>
#include <cmath>

namespace ui {

IconGrid::IconGrid() {}

void IconGrid::setup(std::vector<std::shared_ptr<GlossyIcon>> icons,
                     int cols, int rows,
                     float cellW, float cellH,
                     float padX, float padY)
{
    m_allIcons = std::move(icons);
    m_cols  = cols;  m_rows = rows;
    m_cellW = cellW; m_cellH = cellH;
    m_padX  = padX;  m_padY  = padY;

    int perPage = iconsPerPage();
    m_totalPages = std::max(1, ((int)m_allIcons.size() + perPage - 1) / perPage);

    // Center the grid on screen
    float gridW = m_cols * m_cellW + (m_cols - 1) * m_padX;
    float gridH = m_rows * m_cellH + (m_rows - 1) * m_padY;
    m_originX = (m_rect.width  - gridW) * 0.5f + m_rect.x;
    m_originY = (m_rect.height - gridH) * 0.5f + m_rect.y;

    setPage(0);
}

void IconGrid::setPage(int page) {
    m_page = std::clamp(page, 0, m_totalPages - 1);
    layoutPage();
}

void IconGrid::layoutPage() {
    clearChildren();
    int start = m_page * iconsPerPage();
    int end   = std::min(start + iconsPerPage(), (int)m_allIcons.size());

    std::vector<IFocusable*> fItems;

    for (int i = start; i < end; ++i) {
        auto& icon = m_allIcons[i];
        int local  = i - start;
        int col    = local % m_cols;
        int row    = local / m_cols;
        float x = m_originX + col * (m_cellW + m_padX);
        float y = m_originY + row * (m_cellH + m_padY);
        icon->setRect({x, y, m_cellW, m_cellH});
        addChild(icon);
        fItems.push_back(icon.get());
    }

    m_focus.setGrid(fItems, m_cols);
}

std::vector<GlossyIcon*> IconGrid::pageIcons() const {
    std::vector<GlossyIcon*> out;
    int start = m_page * iconsPerPage();
    int end   = std::min(start + iconsPerPage(), (int)m_allIcons.size());
    for (int i = start; i < end; ++i) out.push_back(m_allIcons[i].get());
    return out;
}

int IconGrid::hitTest(float screenX, float screenY) const {
    int start = m_page * iconsPerPage();
    int end   = std::min(start + iconsPerPage(), (int)m_allIcons.size());
    for (int i = start; i < end; ++i) {
        Rect r = m_allIcons[i]->getFocusRect();
        if (r.contains(screenX, screenY))
            return i - start;
    }
    return -1;
}

void IconGrid::startAppearAnimation() {
    int start = m_page * iconsPerPage();
    int end   = std::min(start + iconsPerPage(), (int)m_allIcons.size());
    // Cascade from top-left to bottom-right: delay based on (col + row)
    int maxDist = (m_cols - 1) + (m_rows - 1);
    for (int i = start; i < end; ++i) {
        int local = i - start;
        int col   = local % m_cols;
        int row   = local / m_cols;
        float t   = maxDist > 0 ? (float)(col + row) / maxDist : 0.f;
        float delay = t * 0.40f;   // 0 → 0.25s spread
        m_allIcons[i]->startAppear(delay);
    }
}

void IconGrid::onUpdate(float /*dt*/) {
    // Children updated by Widget::update
}

void IconGrid::onRender(Renderer& /*ren*/) {
    // Children rendered by Widget::render
}

} // namespace ui
