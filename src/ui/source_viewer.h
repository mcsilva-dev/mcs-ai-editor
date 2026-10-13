#pragma once

#include <QImage>
#include <QWidget>
#include <cstdint>
#include <memory>

class QLabel;
class QSlider;

namespace mcs::media {
class FrameDecoder;
}

namespace mcs::ui {

class FrameView;

// Viewer de fonte: mostra um quadro do clipe e permite escolher o quadro (scrub).
class SourceViewer : public QWidget {
    Q_OBJECT
  public:
    explicit SourceViewer(QWidget* parent = nullptr);
    ~SourceViewer() override;

    // false quando o arquivo não tem vídeo; nesse caso o viewer fica vazio.
    bool openMedia(const QString& path);
    void clear();

    std::int64_t position() const;
    std::int64_t frameCount() const;
    // Limita a [0, frameCount-1].
    void setPosition(std::int64_t frame);
    QImage currentFrame() const;
    QString timecodeText() const;

  private:
    void showFrame(std::int64_t frame);

    std::unique_ptr<media::FrameDecoder> decoder_;
    FrameView* view_ = nullptr;
    QSlider* slider_ = nullptr;
    QLabel* timecode_ = nullptr;
    std::int64_t position_ = 0;
};

} // namespace mcs::ui
