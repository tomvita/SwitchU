// core/Animation.cpp
#include "Animation.hpp"
#include <algorithm>

namespace ui {

AnimationManager& AnimationManager::instance() {
    static AnimationManager inst;
    return inst;
}

void AnimationManager::update(float dt) {
    for (auto& a : m_anims)
        if (a->isRunning()) a->update(dt);
    m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
        [](auto& a){ return a->isFinished(); }), m_anims.end());
}

void AnimationManager::add(std::shared_ptr<Animation> a) { m_anims.push_back(std::move(a)); }
void AnimationManager::clear() { m_anims.clear(); }

} // namespace ui
