#pragma once

#include <QImage>
#include <QWidget>

namespace mcs::ui {

// Exibe um quadro com letterbox em fundo preto.
// v0.2: QPainter na CPU; troca por QRhiWidget no grafo de render do v0.3.
class FrameView : public QWidget {
    Q_OBJECT
  public:
    explicit FrameView(QWidget* parent = nullptr);
    void setImage(const QImage& image);
    QImage image() const;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QImage image_;
};

} // namespace mcs::ui
