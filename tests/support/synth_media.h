#pragma once

#include <QString>
#include <cstdint>

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
    // Formato do FFmpeg: "mov" (indexado), "mp4" (áudio AAC) ou "mpegts" (sem índice; audio=false).
    const char* container = "mov";
    // Capa JPEG marcada AV_DISPOSITION_ATTACHED_PIC (como um M4A/MP3 com capa); exige "mp4".
    bool coverArt = false;
    // Reproduz arquivos com edit list mais curta que as amostras (ex.: clipe cortado no celular):
    // o último quadro fica fora da edição e o demuxer o descarta.
    bool omitPacketDuration = false;
    // Áudio começa N amostras depois do vídeo (gravações de OBS, câmeras, MPEG-TS).
    int audioDelaySamples = 0;
};

// Luma (faixa limitada 16..235) gravado no quadro k.
int lumaForFrame(int k);

// Cinza RGB esperado após decode (faixa limitada → cheia) para o quadro k.
int expectedGrayForFrame(int k);

// Canal esquerdo da amostra n do áudio PCM sintético (o direito é o negativo).
std::int16_t sampleValueAt(std::int64_t n);
// O mesmo valor convertido para float como o FFmpeg/swresample entrega (s16 / 32768).
float expectedSampleAt(std::int64_t n);

// Grava um .mov (mpeg4 + pcm_s16le) usando o FFmpeg linkado. Retorna "" em sucesso.
QString writeSynthMov(const QString& path, const SynthSpec& spec = {});

// Conta pacotes de vídeo e keyframes do arquivo (-1 em erro). Garante que os testes de seek
// exercitam GOP de verdade (não um arquivo só de keyframes).
struct PacketStats {
    int packets = -1;
    int keyframes = -1;
    int discarded = -1; // fora da edit list do .mov: o decoder os descarta
};
PacketStats videoPacketStats(const QString& path);

// true se algum stream do arquivo é uma capa (AV_DISPOSITION_ATTACHED_PIC).
bool hasAttachedPicture(const QString& path);

} // namespace mcs::test
