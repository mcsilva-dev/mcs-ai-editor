#include "ui/theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace mcs::ui {

void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette p;
    p.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::WindowText, QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::Base, QColor(0x16, 0x16, 0x16));
    p.setColor(QPalette::AlternateBase, QColor(0x23, 0x23, 0x23));
    p.setColor(QPalette::Text, QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::Button, QColor(0x2a, 0x2a, 0x2a));
    p.setColor(QPalette::ButtonText, QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::ToolTipBase, QColor(0x2a, 0x2a, 0x2a));
    p.setColor(QPalette::ToolTipText, QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::Highlight, QColor(0xe0, 0x7a, 0x2e));
    p.setColor(QPalette::HighlightedText, QColor(0x10, 0x10, 0x10));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x70, 0x70, 0x70));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x70, 0x70, 0x70));
    app.setPalette(p);
}

} // namespace mcs::ui
