
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "exrmetrics.h"

#include "ImfChannelList.h"
#include "ImfDeepFrameBuffer.h"
#include "ImfDeepScanLineInputPart.h"
#include "ImfDeepScanLineOutputPart.h"
#include "ImfDeepTiledInputPart.h"
#include "ImfDeepTiledOutputPart.h"
#include "ImfHeader.h"
#include "ImfInputPart.h"
#include "ImfMisc.h"
#include "ImfMultiPartInputFile.h"
#include "ImfMultiPartOutputFile.h"
#include "ImfOutputPart.h"
#include "ImfPartType.h"
#include "ImfTiledInputPart.h"
#include "ImfTiledOutputPart.h"

#include <chrono>
#include <ctime>
#include <list>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>

using namespace Imf;
using Imath::Box2i;

using std::cerr;
using namespace std::chrono;
using std::chrono::steady_clock;
using std::cout;
using std::endl;
using std::list;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;

// Backport from 3.3.1
#if OPENEXR_VERSION_MINOR < 3

/// Store codec properties so they may be queried in various places.
struct CompressionDesc
{
    std::string name;         // short name
    std::string desc;         // method description
    int         numScanlines; // number of scanlines required
    bool        lossy;        // true if lossy algorithm
    bool        deep;         // true is capable of compressing deep data

    CompressionDesc (
        std::string _name,
        std::string _desc,
        int         _scanlines,
        bool        _lossy,
        bool        _deep)
    {
        name         = _name;
        desc         = _desc;
        numScanlines = _scanlines;
        lossy        = _lossy;
        deep         = _deep;
    }
};

// NOTE: IdToDesc order MUST match Imf::Compression enum.
// clang-format off
static const CompressionDesc IdToDesc[] = {
    CompressionDesc (
        "none",
        "no compression.",
        1,
        false,
        true),
    CompressionDesc (
        "rle",
        "run-length encoding.",
        1,
        false,
        true),
    CompressionDesc (
        "zips",
        "zlib compression, one scan line at a time.",
        1,
        false,
        true),
    CompressionDesc (
        "zip",
        "zlib compression, in blocks of 16 scan lines.",
        16,
        false,
        false),
    CompressionDesc (
        "piz",
        "piz-based wavelet compression, in blocks of 32 scan lines.",
        32,
        false,
        false),
    CompressionDesc (
        "pxr24",
        "lossy 24-bit float compression, in blocks of 16 scan lines.",
        16,
        true,
        false),
    CompressionDesc (
        "b44",
        "lossy 4-by-4 pixel block compression, fixed compression rate.",
        32,
        true,
        false),
    CompressionDesc (
        "b44a",
        "lossy 4-by-4 pixel block compression, flat fields are compressed more.",
        32,
        true,
        false),
    CompressionDesc (
        "dwaa",
        "lossy DCT based compression, in blocks of 32 scanlines. More efficient "
        "for partial buffer access.",
        32,
        true,
        false),
    CompressionDesc (
        "dwab",
        "lossy DCT based compression, in blocks of 256 scanlines. More efficient "
        "space wise and faster to decode full frames than DWAA_COMPRESSION.",
        256,
        true,
        false),
};
// clang-format on

// NOTE: CompressionNameToId order MUST match Imf::Compression enum.
static const std::map<std::string, Compression> CompressionNameToId = {
    {"no", Compression::NO_COMPRESSION},
    {"none", Compression::NO_COMPRESSION},
    {"rle", Compression::RLE_COMPRESSION},
    {"zips", Compression::ZIPS_COMPRESSION},
    {"zip", Compression::ZIP_COMPRESSION},
    {"piz", Compression::PIZ_COMPRESSION},
    {"pxr24", Compression::PXR24_COMPRESSION},
    {"b44", Compression::B44_COMPRESSION},
    {"b44a", Compression::B44A_COMPRESSION},
    {"dwaa", Compression::DWAA_COMPRESSION},
    {"dwab", Compression::DWAB_COMPRESSION},
};

#define UNKNOWN_COMPRESSION_ID_MSG "INVALID COMPRESSION ID"

/// Returns a codec ID's short name (lowercase).
void
getCompressionNameFromId (Compression id, std::string& name)
{
    if (id < NO_COMPRESSION || id >= NUM_COMPRESSION_METHODS)
        name = UNKNOWN_COMPRESSION_ID_MSG;
    name = IdToDesc[static_cast<int> (id)].name;
}

/// Return the number of scan lines expected by a given compression method.
int
getCompressionNumScanlines (Compression id)
{
    if (id < NO_COMPRESSION || id >= NUM_COMPRESSION_METHODS) return -1;
    return IdToDesc[static_cast<int> (id)].numScanlines;
}

/// Returns the codec name's ID, NUM_COMPRESSION_METHODS if not found.
void
getCompressionIdFromName (const std::string& name, Compression& id)
{
    std::string lowercaseName (name);
    for (auto& ch: lowercaseName)
        ch = std::tolower (ch);

    auto it = CompressionNameToId.find (lowercaseName);
    id      = it != CompressionNameToId.end ()
                  ? it->second
                  : Compression::NUM_COMPRESSION_METHODS;
}

/// Return a string enumerating all compression names, with a custom separator.
void
getCompressionNamesString (const std::string& separator, std::string& str)
{
    int i = 0;
    for (; i < static_cast<int> (NUM_COMPRESSION_METHODS) - 1; i++)
    {
        str += IdToDesc[i].name + separator;
    }
    str += IdToDesc[i].name;
}

#endif


// ----8<----
// Taken from ImfTiledMisc.cpp to allow standalone compilation
using IMATH_NAMESPACE::V2i;

int
levelSize (int min, int max, int l, LevelRoundingMode rmode)
{
    if (l < 0) throw IEX_NAMESPACE::ArgExc ("Argument not in valid range.");

    int a    = max - min + 1;
    int b    = (1 << l);
    int size = a / b;

    if (rmode == ROUND_UP && size * b < a) size += 1;

    return std::max (size, 1);
}

Box2i
dataWindowForLevel (
    const TileDescription& tileDesc,
    int                    minX,
    int                    maxX,
    int                    minY,
    int                    maxY,
    int                    lx,
    int                    ly)
{
    V2i levelMin = V2i (minX, minY);

    V2i levelMax =
        levelMin + V2i (
                       levelSize (minX, maxX, lx, tileDesc.roundingMode) - 1,
                       levelSize (minY, maxY, ly, tileDesc.roundingMode) - 1);

    return Box2i (levelMin, levelMax);
}

// ----8<----

double
timing (steady_clock::time_point start, steady_clock::time_point end)
{
    return std::chrono::duration<double>(end-start).count();
}

int
channelCount (const Header& h)
{
    int channels = 0;
    for (ChannelList::ConstIterator i = h.channels ().begin ();
         i != h.channels ().end ();
         ++i)
    {
        ++channels;
    }
    return channels;
}

void
copyScanLine (InputPart& in, OutputPart& out)
{
    Box2i    dw        = in.header ().dataWindow ();
    uint64_t width     = dw.max.x + 1 - dw.min.x;
    uint64_t height    = dw.max.y + 1 - dw.min.y;
    uint64_t numPixels = width * height;
    int      numChans  = channelCount (in.header ());

    vector<vector<char>> pixelData (numChans);
    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);

    int         channelNumber = 0;
    int         pixelSize     = 0;
    FrameBuffer buf;

    for (ChannelList::ConstIterator i = out.header ().channels ().begin ();
         i != out.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        pixelData[channelNumber].resize (numPixels * samplesize);

        buf.insert (
            i.name (),
            Slice (
                i.channel ().type,
                pixelData[channelNumber].data () - offsetToOrigin * samplesize,
                samplesize,
                samplesize * width));
        ++channelNumber;
        pixelSize += samplesize;
    }

    in.setFrameBuffer (buf);
    out.setFrameBuffer (buf);

    steady_clock::time_point startRead = steady_clock::now();
    in.readPixels (dw.min.y, dw.max.y);
    steady_clock::time_point endRead = steady_clock::now();

    steady_clock::time_point startWrite = steady_clock::now();
    out.writePixels (height);
    steady_clock::time_point endWrite = steady_clock::now();

    cout << "   \"read time\": " << timing (startRead, endRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"pixel count\": " << numPixels << ",\n";
    cout << "   \"raw size\": " << numPixels * pixelSize << ",\n";
}

void
copyTiled (TiledInputPart& in, TiledOutputPart& out)
{
    int             numChans = channelCount (in.header ());
    TileDescription tiling   = in.header ().tileDescription ();

    Box2i imageDw = in.header ().dataWindow ();
    int   totalLevels;
    switch (tiling.mode)
    {
        case ONE_LEVEL: totalLevels = 1; //break;
        case MIPMAP_LEVELS: totalLevels = in.numLevels (); break;
        case RIPMAP_LEVELS:
            totalLevels = in.numXLevels () * in.numYLevels ();
            break;
        case NUM_LEVELMODES:
        default:
            throw runtime_error ("unknown tile mode");
    }

    vector<vector<vector<char>>> pixelData (totalLevels);
    vector<FrameBuffer>          frameBuffer (totalLevels);

    int    levelIndex  = 0;
    int    pixelSize   = 0;
    size_t totalPixels = 0;

    //
    // allocate memory and initialize frameBuffer for each level
    //
    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                Box2i dw = dataWindowForLevel (
                    tiling,
                    imageDw.min.x,
                    imageDw.max.x,
                    imageDw.min.y,
                    imageDw.max.y,
                    xLevel,
                    yLevel);
                uint64_t width     = dw.max.x + 1 - dw.min.x;
                uint64_t height    = dw.max.y + 1 - dw.min.y;
                uint64_t numPixels = width * height;
                uint64_t offsetToOrigin =
                    width * static_cast<uint64_t> (dw.min.y) +
                    static_cast<uint64_t> (dw.min.x);
                int channelNumber = 0;
                pixelSize         = 0;

                pixelData[levelIndex].resize (numChans);

                for (ChannelList::ConstIterator i =
                         out.header ().channels ().begin ();
                     i != out.header ().channels ().end ();
                     ++i)
                {
                    int samplesize = pixelTypeSize (i.channel ().type);
                    pixelData[levelIndex][channelNumber].resize (
                        numPixels * samplesize);

                    frameBuffer[levelIndex].insert (
                        i.name (),
                        Slice (
                            i.channel ().type,
                            pixelData[levelIndex][channelNumber].data () -
                                offsetToOrigin * samplesize,
                            samplesize,
                            samplesize * width));
                    ++channelNumber;
                    pixelSize += samplesize;
                }
                totalPixels += numPixels;
                ++levelIndex;
            }
        }
    }

    steady_clock::time_point startRead = steady_clock::now();
    levelIndex        = 0;

    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                in.setFrameBuffer (frameBuffer[levelIndex]);
                in.readTiles (
                    0,
                    in.numXTiles (xLevel) - 1,
                    0,
                    in.numYTiles (yLevel) - 1,
                    xLevel,
                    yLevel);
                ++levelIndex;
            }
        }
    }

    steady_clock::time_point endRead = steady_clock::now();

    steady_clock::time_point startWrite = steady_clock::now();
    levelIndex         = 0;
    int tileCount      = 0;

    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                out.setFrameBuffer (frameBuffer[levelIndex]);
                out.writeTiles (
                    0,
                    in.numXTiles (xLevel) - 1,
                    0,
                    in.numYTiles (yLevel) - 1,
                    xLevel,
                    yLevel);
                tileCount += in.numXTiles (xLevel) * in.numYTiles (yLevel);
                ++levelIndex;
            }
        }
    }
    steady_clock::time_point endWrite = steady_clock::now();

    cout << "   \"read time\": " << timing (startRead, endRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"total tiles\": " << tileCount << ",\n";
    cout << "   \"pixel count\": " << totalPixels << ",\n";
    cout << "   \"raw size\": " << totalPixels * pixelSize << ",\n";
}

void
copyDeepScanLine (DeepScanLineInputPart& in, DeepScanLineOutputPart& out)
{
    Box2i       dw        = in.header ().dataWindow ();
    uint64_t    width     = dw.max.x + 1 - dw.min.x;
    uint64_t    height    = dw.max.y + 1 - dw.min.y;
    uint64_t    numPixels = width * height;
    int         numChans  = channelCount (in.header ());
    vector<int> sampleCount (numPixels);

    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);
    vector<vector<char*>> pixelPtrs (numChans);

    DeepFrameBuffer buffer;

    buffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (sampleCount.data () - offsetToOrigin),
        sizeof (int),
        sizeof (int) * width));
    int channelNumber  = 0;
    int bytesPerSample = 0;
    for (ChannelList::ConstIterator i = out.header ().channels ().begin ();
         i != out.header ().channels ().end ();
         ++i)
    {
        pixelPtrs[channelNumber].resize (numPixels);
        int samplesize = pixelTypeSize (i.channel ().type);
        buffer.insert (
            i.name (),
            DeepSlice (
                i.channel ().type,
                (char*) (pixelPtrs[channelNumber].data () - offsetToOrigin),
                sizeof (char*),
                sizeof (char*) * width,
                samplesize));
        ++channelNumber;
        bytesPerSample += samplesize;
    }

    in.setFrameBuffer (buffer);
    out.setFrameBuffer (buffer);

    steady_clock::time_point startCountRead = steady_clock::now();
    in.readPixelSampleCounts (dw.min.y, dw.max.y);
    steady_clock::time_point endCountRead = steady_clock::now();

    size_t totalSamples = 0;

    for (int i: sampleCount)
    {
        totalSamples += i;
    }

    vector<vector<char>> sampleData (numChans);
    channelNumber = 0;
    for (ChannelList::ConstIterator i = in.header ().channels ().begin ();
         i != in.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        sampleData[channelNumber].resize (samplesize * totalSamples);
        int offset = 0;
        for (uint64_t p = 0; p < numPixels; ++p)
        {
            pixelPtrs[channelNumber][p] =
                sampleData[channelNumber].data () + offset * samplesize;
            offset += sampleCount[p];
        }

        ++channelNumber;
    }

    steady_clock::time_point startSampleRead = steady_clock::now();
    in.readPixels (dw.min.y, dw.max.y);
    steady_clock::time_point endSampleRead = steady_clock::now();


    steady_clock::time_point startWrite = steady_clock::now();
    out.writePixels (height);
    steady_clock::time_point endWrite = steady_clock::now();


    cout << "   \"count read time\": " << timing (startCountRead, endCountRead)
         << ",\n";
    cout << "   \"sample read time\": "
         << timing (startSampleRead, endSampleRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"pixel count\": " << numPixels << ",\n";
    cout << "   \"raw size\": "
         << totalSamples * bytesPerSample + numPixels * sizeof (int) << ",\n";
}

void
copyDeepTiled (DeepTiledInputPart& in, DeepTiledOutputPart& out)
{

    TileDescription tiling = in.header ().tileDescription ();

    if (tiling.mode == MIPMAP_LEVELS)
    {
        throw runtime_error (
            "exrmetrics does not support mipmapped deep tiled parts");
    }

    if (tiling.mode == RIPMAP_LEVELS)
    {
        throw runtime_error (
            "exrmetrics does not support ripmapped deep tiled parts");
    }

    Box2i       dw        = in.header ().dataWindow ();
    uint64_t    width     = dw.max.x + 1 - dw.min.x;
    uint64_t    height    = dw.max.y + 1 - dw.min.y;
    uint64_t    numPixels = width * height;
    int         numChans  = channelCount (in.header ());
    vector<int> sampleCount (numPixels);

    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);
    vector<vector<char*>> pixelPtrs (numChans);

    DeepFrameBuffer buffer;

    buffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (sampleCount.data () - offsetToOrigin),
        sizeof (int),
        sizeof (int) * width));
    int channelNumber  = 0;
    int bytesPerSample = 0;
    for (ChannelList::ConstIterator i = out.header ().channels ().begin ();
         i != out.header ().channels ().end ();
         ++i)
    {
        pixelPtrs[channelNumber].resize (numPixels);
        int samplesize = pixelTypeSize (i.channel ().type);
        buffer.insert (
            i.name (),
            DeepSlice (
                i.channel ().type,
                (char*) (pixelPtrs[channelNumber].data () - offsetToOrigin),
                sizeof (char*),
                sizeof (char*) * width,
                samplesize));
        ++channelNumber;
        bytesPerSample += samplesize;
    }

    in.setFrameBuffer (buffer);
    out.setFrameBuffer (buffer);

    steady_clock::time_point startCountRead = steady_clock::now();

    in.readPixelSampleCounts (
        0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endCountRead = steady_clock::now();


    size_t totalSamples = 0;

    for (int i: sampleCount)
    {
        totalSamples += i;
    }

    vector<vector<char>> sampleData (numChans);
    channelNumber = 0;
    for (ChannelList::ConstIterator i = in.header ().channels ().begin ();
         i != in.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        sampleData[channelNumber].resize (samplesize * totalSamples);
        int offset = 0;
        for (uint64_t p = 0; p < numPixels; ++p)
        {
            pixelPtrs[channelNumber][p] =
                sampleData[channelNumber].data () + offset * samplesize;
            offset += sampleCount[p];
        }

        ++channelNumber;
    }

    steady_clock::time_point startSampleRead = steady_clock::now();
    in.readTiles (0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endSampleRead = steady_clock::now();

    steady_clock::time_point startWrite = steady_clock::now();
    out.writeTiles (0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endWrite = steady_clock::now();


    cout << "   \"count read time\": " << timing (startCountRead, endCountRead)
         << ",\n";
    cout << "   \"sample read time\": "
         << timing (startSampleRead, endSampleRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"pixel count\": " << numPixels << ",\n";
    cout << "   \"raw size\": "
         << totalSamples * bytesPerSample + numPixels * sizeof (int) << ",\n";
}

void
exrmetrics (
    const char       inFileName[],
    const char       outFileName[],
    int              part,
    Imf::Compression compression,
    float            level,
    int              halfMode)
{
    MultiPartInputFile in (inFileName);
    if (part >= in.parts ())
    {
        throw runtime_error ((string (inFileName) + " only contains " +
                              to_string (in.parts ()) +
                              " parts. Cannot copy part " + to_string (part))
                                 .c_str ());
    }
    Header outHeader = in.header (part);

    if (compression < NUM_COMPRESSION_METHODS)
    {
        outHeader.compression () = compression;
    }
    else { compression = outHeader.compression (); }

    if (!isinf (level) && level >= -1)
    {
        switch (outHeader.compression ())
        {
            case DWAA_COMPRESSION:
            case DWAB_COMPRESSION:
                outHeader.dwaCompressionLevel () = level;
                break;
            case ZIP_COMPRESSION:
            case ZIPS_COMPRESSION:
                outHeader.zipCompressionLevel () = level;
                break;
                //            case ZSTD_COMPRESSION :
                //                outHeader.zstdCompressionLevel()=level;
                //                break;
            default:
                throw runtime_error (
                    "-l option only works for DWAA/DWAB,ZIP/ZIPS or ZSTD compression");
        }
    }

    if (halfMode > 0)
    {
        for (ChannelList::Iterator i = outHeader.channels ().begin ();
             i != outHeader.channels ().end ();
             ++i)
        {
            if (halfMode == 2 || !strcmp (i.name (), "R") ||
                !strcmp (i.name (), "G") || !strcmp (i.name (), "B") ||
                !strcmp (i.name (), "A"))
            {
                i.channel ().type = HALF;
            }
        }
    }

    string inCompress, outCompress;
    getCompressionNameFromId (in.header (part).compression (), inCompress);
    getCompressionNameFromId (outHeader.compression (), outCompress);
    cout << "{\n";
    cout << "   \"input compression\": \"" << inCompress << "\",\n";
    cout << "   \"output compression\": \"" << outCompress << "\",\n";
    if (compression == ZIP_COMPRESSION || compression == ZIPS_COMPRESSION)
    {
        cout << "   \"zipCompressionLevel\": "
             << outHeader.zipCompressionLevel () << ",\n";
    }

    if (compression == DWAA_COMPRESSION || compression == DWAB_COMPRESSION)
    {
        cout << "   \"dwaCompressionLevel\": "
             << outHeader.dwaCompressionLevel () << ",\n";
    }

    std::string type = outHeader.type ();
    cout << "   \"part type\": \"" << type << "\",\n";

    if (type == SCANLINEIMAGE)
    {
        cout << "   \"scanlines per chunk:\" : "
             << getCompressionNumScanlines (compression) << ",\n";
    }

    {
        MultiPartOutputFile out (outFileName, &outHeader, 1);

        if (type == TILEDIMAGE)
        {
            TiledInputPart  inpart (in, part);
            TiledOutputPart outpart (out, 0);
            copyTiled (inpart, outpart);
        }
        else if (type == SCANLINEIMAGE)
        {
            InputPart  inpart (in, part);
            OutputPart outpart (out, 0);
            copyScanLine (inpart, outpart);
        }
        else if (type == DEEPSCANLINE)
        {
            DeepScanLineInputPart  inpart (in, part);
            DeepScanLineOutputPart outpart (out, 0);
            copyDeepScanLine (inpart, outpart);
        }
        else if (type == DEEPTILE)
        {
            DeepTiledInputPart  inpart (in, part);
            DeepTiledOutputPart outpart (out, 0);
            copyDeepTiled (inpart, outpart);
        }
        else
        {
            throw runtime_error (
                (inFileName + string (" contains unknown part type ") + type)
                    .c_str ());
        }
    }
    struct stat instats, outstats;
    stat (inFileName, &instats);
    stat (outFileName, &outstats);
    cout << "   \"input file size\": " << instats.st_size << ",\n";
    cout << "   \"output file size\": " << outstats.st_size << "\n";
    cout << "}\n";
}
