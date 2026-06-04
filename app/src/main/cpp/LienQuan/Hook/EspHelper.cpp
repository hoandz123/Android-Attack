#include "EspHelper.h"
#include "EspIconView.h"
#include "EspRuntime.h"
#include "../Config/Config.h"
#include <DrawRender.hpp>
#include <GameUI/EspGUI.h>
#include <Includes/obfuscate.h>
#include <cstdio>
#include <imgui.h>

namespace lienquan {
namespace EspHelper {

namespace Draw {
namespace {

void DrawHostDebug(ImDrawList *dl, const EspRuntime::Snapshot &s) {
    char buf[96]{};
    snprintf(buf, sizeof(buf), OBF("myCamp=%d enemy=%d hpid=%u metaPid=%u seq=%u"),
             s.myCamp, s.myEnemyCamp, s.dbgHostPid, s.dbgMetaPid, s.seq);
    dl->AddText(ImVec2(8.f, 8.f), IM_COL32(120, 220, 255, 255), buf);
}

void DrawLine(ImDrawList *dl, const EspRuntime::LineItem &line, float thick) {
    if (!line.valid) return;
    dl->AddLine({line.fromX, line.fromY}, {line.toX, line.toY}, EspGUI::kEspLineColor, thick);
}

void DrawMinimapDot(ImDrawList *dl, const EspRuntime::MapItem &item, float dotR) {
    if (!item.valid) return;
    dl->AddCircleFilled({item.x, item.y}, dotR, IM_COL32(255, 220, 0, 240));
    if (!gLQConfig.esp.showDebug) return;
    char buf[80]{};
    snprintf(buf, sizeof(buf), OBF("mm %.0f,%.0f id=%u"), item.x, item.y, item.objId);
    dl->AddText(ImVec2(item.x + 5.f, item.y - 10.f), IM_COL32(255, 220, 0, 255), buf);
}

void DrawWorldDebug(ImDrawList *dl, const EspRuntime::Snapshot &s, int i) {
    if (i < 0 || i >= s.targetCount || !s.lines[i].valid) return;
    const float x = s.lines[i].toX, y = s.lines[i].toY;
    dl->AddCircleFilled({x, y}, 4.f, IM_COL32(255, 220, 0, 255));
    const Vector3 &w = s.targetWorld[i];
    char buf[96]{};
    snprintf(buf, sizeof(buf), OBF("%.1f,%.1f,%.1f | %.0f,%.0f | c=%d"),
             w.x, w.y, w.z, x, y, s.targetCamp[i]);
    dl->AddText(ImVec2(x + 6.f, y - 14.f), IM_COL32(255, 255, 255, 255), buf);
}

void DrawMapModeDebug(ImDrawList *dl, EspRuntime::Snapshot &s) {
    char buf[48]{};
    snprintf(buf, sizeof(buf), OBF("minimap mode=%d"), static_cast<int>(s.mapMode));
    dl->AddText(ImVec2(8.f, 24.f), IM_COL32(255, 220, 0, 255), buf);
}

} // namespace

void Overlay() {
    if (!gLQConfig.esp.enabled && !gLQConfig.esp.minimapDot && !gLQConfig.esp.showHeroIcons &&
        !gLQConfig.esp.showDebug)
        return;
    EspRuntime::Snapshot snap{};
    if (!EspRuntime::ReadSnapshot(snap)) return;
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    if (gLQConfig.esp.showHeroIcons) EspIconView::Draw(dl, snap);

    const bool wantLine = gLQConfig.esp.enabled && snap.hasMyWorld && snap.targetCount > 0;
    const bool wantMm = gLQConfig.esp.minimapDot && snap.targetCount > 0;
    if (!wantLine && !wantMm) {
        if (gLQConfig.esp.showDebug) DrawHostDebug(dl, snap);
        return;
    }
    const float thick = gLQConfig.esp.lineThickness > 0.f ? gLQConfig.esp.lineThickness : EspGUI::kDefaultLineThickness;
    const float dotR = gLQConfig.esp.minimapDotRadius > 0.f ? gLQConfig.esp.minimapDotRadius : 3.f;
    if (gLQConfig.esp.showDebug) DrawHostDebug(dl, snap);
    for (int i = 0; i < snap.targetCount && i < EspRuntime::kMaxTargets; ++i) {
        if (wantLine) DrawLine(dl, snap.lines[i], thick);
        if (wantMm) DrawMinimapDot(dl, snap.mapItems[i], dotR);
        if (gLQConfig.esp.showDebug && wantLine) DrawWorldDebug(dl, snap, i);
    }
    if (gLQConfig.esp.showDebug && wantMm && snap.mapMode != MinimapSys::EMapType::None) DrawMapModeDebug(dl, snap);
}

void Register() {
    static bool once = false;
    if (once) return;
    DrawRender::registerTask(Overlay);
    once = true;
}

} // namespace Draw

}
}
