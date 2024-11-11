
#ifndef INCLUDED_EXR_METRICS_H
#define INCLUDED_EXR_METRICS_H

//----------------------------------------------------------------------------
//
//	Copy input to output, reporting file size and timing
//
//----------------------------------------------------------------------------

#include "ImfCompression.h"

// Backport from 3.3.1
#if OPENEXR_VERSION_MINOR < 3
#include <string>

/// Returns a codec ID's short name (lowercase).
void getCompressionNameFromId (Imf::Compression id, std::string& name);

/// Return the number of scan lines expected by a given compression method.
IMF_EXPORT int getCompressionNumScanlines (Imf::Compression id);

/// Returns the codec name's ID, NUM_COMPRESSION_METHODS if not found.
IMF_EXPORT void
getCompressionIdFromName (const std::string& name, Imf::Compression& id);

/// Return a string enumerating all compression names, with a custom separator.
IMF_EXPORT void
getCompressionNamesString (const std::string& separator, std::string& in);

#endif

void exrmetrics (
    const char       inFileName[],
    const char       outFileName[],
    int              part,
    Imf::Compression compression,
    float            level,
    int              halfMode);
#endif
