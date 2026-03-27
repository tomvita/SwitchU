// core/Animation.cpp
#include <nxui/core/Animation.hpp>
#include <algorithm>

namespace nxui {

AnimationManager& AnimationManager::instance() {
    static AnimationManager inst;
    return inst;
}

void AnimationManager::update(float dt) {
    // Snapshot size — callbacks may add new animations via push_back.
    // New animations added during this loop will be past `n` and not iterated.
    size_t n = m_anims.size();
    for (size_t i = 0; i < n; ++i) {
        if (m_anims[i]->isRunning())
            m_anims[i]->update(dt);
    }
    m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
        [](auto& a){ return a->isFinished(); }), m_anims.end());
}

void AnimationManager::add(std::shared_ptr<Animation> a) { m_anims.push_back(std::move(a)); }
void AnimationManager::clear() { m_anims.clear(); }

} // namespace nxui
