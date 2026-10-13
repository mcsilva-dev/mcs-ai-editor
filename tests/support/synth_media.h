#pragma once

#include <QString>

namespace mcs::test {

// Vídeo cinza cujo brilho codifica o número do quadro; áudio estéreo em silêncio.
struct SynthSpec {
    int frames = 60;
    int fps = 25;
    int width = 64;
    int height = 48;
    int gop = 12; // keyframe a cada 12 quadros: força seek no meio do GOP
    bool video = true;
    bool audio = true;
};

// Luma (faixa limitada 16..235) gravado no quadro k.
int lumaForFrame(int k);

// Cinza RGB esperado após decode (faixa limitada → cheia) para o quadro k.
int expectedGrayForFrame(int k);

// Grava um .mov (mpeg4 + pcm_s16le) usando o FFmpeg linkado. Retorna "" em sucesso.
QString writeSynthMov(const QString& path, const SynthSpec& spec = {});

} // namespace mcs::test
