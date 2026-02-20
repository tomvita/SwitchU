#pragma once
#include "Widget.hpp"
#include "FocusManager.hpp"
#include "../core/Types.hpp"
#include <vector>
#include <memory>

namespace ui {

class GlossyIcon;
class Renderer;

class IconGrid : public Widget {
public:
    IconGrid();

    void setup(std::vector<std::shared_ptr<GlossyIcon>> icons,
               int cols, int rows,
               float cellW, float cellH,
               float padX, float padY);

    void setPage(int page);
    int  currentPage()  const { return m_page; }
    int  totalPages()   const { return m_totalPages; }
    int  columns()      const { return m_cols; }
    int  rowsPerPage()  const { return m_rows; }
    int  iconsPerPage() const { return m_cols * m_rows; }

    FocusManager& focusManager() { return m_focus; }
    const std::vector<std::shared_ptr<GlossyIcon>>& allIcons() const { return m_allIcons; }

    // Returns icons on the current page
    std::vector<GlossyIcon*> pageIcons() const;

    /// Returns the page-local focus index of the icon at screen position, or -1
    int hitTest(float screenX, float screenY) const;

    void startAppearAnimation();

protected:
    void onUpdate(float dt) override;
    void onRender(Renderer& ren) override;

private:
    void layoutPage();

    std::vector<std::shared_ptr<GlossyIcon>> m_allIcons;
    FocusManager m_focus;

    int m_cols = 5, m_rows = 3;
    int m_page = 0, m_totalPages = 1;
    float m_cellW = 200, m_cellH = 200;
    float m_padX  = 20,  m_padY  = 20;
    float m_originX = 0, m_originY = 0;
};

} // namespace ui
