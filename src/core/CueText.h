#pragma once

#include <QString>

namespace afvp {

// Removes HTML-like tags used by SRT and WebVTT (<i>, <b>, <font ...>, <c.class>, <v Name>, <00:01.000> ...).
QString removeMarkupTags(const QString &text);

// Removes ASS override blocks like {\an8} that are often found in SRT files.
QString removeAssOverrideBlocks(const QString &text);

// Decodes HTML entities used by WebVTT: &amp; &lt; &gt; &nbsp; &quot; &apos; &lrm; &rlm; &#NNN; &#xHH;.
QString decodeHtmlEntities(const QString &text);

// Trims every line, drops empty lines and joins them with '\n'. Returns an empty string for a blank cue.
QString cleanCueLines(const QString &text);

// Text of a cue for the subtitle list (I-20): line breaks become spaces, except before a line that starts
// with a dialogue dash; repeated spaces are collapsed.
QString normalizeCueText(const QString &text);

} // namespace afvp
