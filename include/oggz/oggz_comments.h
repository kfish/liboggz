/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __OGGZ_COMMENT_H__
#define __OGGZ_COMMENT_H__

/*
 * XXX: oggz_comment_generate() IS NOT YET IMPLEMENTED
 * - When writing, Oggz allows you to set up the comments in memory, and
 *   provides a single function to generate a corresponding ogg_packet. 
 *   It is your responsibility to then actually write that packet in sequence.
 *
 * \section comments_set Writing comments
 * 
 * For writing, Oggz contains API methods for adding comments
 * (oggz_comment_add() and oggz_comment_add_byname()
 * and for removing comments
 * (oggz_comment_remove() and oggz_comment_remove_byname()).
 * XXX: There is currently no function to write the comments to the bitstream!
 */

/** \file
 * Reading of comments.
 *
 * Vorbis, Speex and Theora bitstreams
 * use a comment format called "Vorbiscomment", defined 
 * <a href="http://www.xiph.org/ogg/vorbis/doc/v-comment.html">here</a>.
 * Many standard comment names (such as TITLE, COPYRIGHT and GENRE) are
 * defined in that document.
 *
 * The following general features of Vorbiscomment are relevant to this API:
 * - Each stream has one comment packet, which occurs before any encoded
 *   audio data in the stream.
 * - When reading, Oggz will decode the comment block before calling
 *   the second read() callback for each stream. Hence, retrieving comment
 *   data is possible once the read() callback has been called a second time.
 *
 * Each comment block contains one Vendor string, which can be retrieved
 * with oggz_comment_get_vendor().
 *
 * The rest of a comment block consists of \a name = \a value pairs, with
 * the following restrictions:
 * - Both the \a name and \a value must be non-empty
 * - The \a name is case-insensitive and must consist of ASCII within the
 *   range 0x20 to 0x7D inclusive, 0x3D ('=') excluded.
 * - The \a name is not unique; multiple entries may exist with equivalent
 *   \a name within a Vorbiscomment block.
 * - The \a value may be any UTF-8 string.
 *
 * \section comments_get Reading comments
 *
 * Oggz contains API methods to iterate through all comments associated
 * with the logical bitstreams of an OGGZ* handle (oggz_comment_first() and
 * oggz_comment_next(), and to iterate through comments matching a
 * particular name (oggz_comment_first_byname() and
 * oggz_comment_next_byname()). Given that multiple comments may exist
 * with the same \a name, you should not use
 * oggz_comment_first_byname() as a simple "get" function.
 *
 */

#include <oggz/oggz.h>

/**
 * A comment.
 */
typedef struct {
  /** The name of the comment, eg. "AUTHOR" */
  char * name;

  /** The value of the comment, as UTF-8 */
  char * value;
} OggzComment;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieve the vendor string.
 * \param oggz A OGGZ* handle
 * \param serialno Identify a logical bitstream within \a oggz
 * \returns A read-only copy of the vendor string.
 * \retval NULL No vendor string is associated with \a oggz,
 *              or \a oggz is NULL, or \a serialno does not identify an
 *              existing logical bitstream in \a oggz.
 */
const char *
oggz_comment_get_vendor (OGGZ * oggz, long serialno);


/**
 * Retrieve the first comment.
 * \param oggz A OGGZ* handle
 * \param serialno Identify a logical bitstream within \a oggz
 * \returns A read-only copy of the first comment.
 * \retval NULL No comments exist for this OGGZ* object, or \a serialno
 *              does not identify an existing logical bitstream in \a oggz.
 */
const OggzComment *
oggz_comment_first (OGGZ * oggz, long serialno);

/**
 * Retrieve the next comment.
 * \param oggz A OGGZ* handle
 * \param serialno Identify a logical bitstream within \a oggz
 * \param comment The previous comment.
 * \returns A read-only copy of the comment immediately following the given
 *          comment.
 * \retval NULL \a serialno does not identify an existing
 *              logical bitstream in \a oggz.
 */
const OggzComment *
oggz_comment_next (OGGZ * oggz, long serialno, const OggzComment * comment);

/**
 * Retrieve the first comment with a given name.
 * \param oggz A OGGZ* handle
 * \param serialno Identify a logical bitstream within \a oggz
 * \param name the name of the comment to retrieve.
 * \returns A read-only copy of the first comment matching the given \a name.
 * \retval NULL No match was found, or \a serialno does not identify an
 *              existing logical bitstream in \a oggz.
 * \note If \a name is NULL, the behaviour is the same as for
 *       oggz_comment_first()
 */
const OggzComment *
oggz_comment_first_byname (OGGZ * oggz, long serialno, char * name);

/**
 * Retrieve the next comment following and with the same name as a given
 * comment.
 * \param oggz A OGGZ* handle
 * \param serialno Identify a logical bitstream within \a oggz
 * \param comment A comment
 * \returns A read-only copy of the next comment with the same name as
 *          \a comment.
 * \retval NULL No further comments with the same name exist for this
 *              OGGZ* object, or \a serialno does not identify an existing
 *              logical bitstream in \a oggz.
 */
const OggzComment *
oggz_comment_next_byname (OGGZ * oggz, long serialno,
                          const OggzComment * comment);

/**
 * Add a comment
 * \param oggz A OGGZ* handle (created with mode OGGZ_ENCODE)
 * \param serialno Identify a logical bitstream within \a oggz
 * \param comment The comment to add
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD \a oggz is not a valid OGGZ* handle
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
int
oggz_comment_add (OGGZ * oggz, long serialno, OggzComment * comment);

/**
 * Add a comment by name and value.
 * \param oggz A OGGZ* handle (created with mode OGGZ_ENCODE)
 * \param serialno Identify a logical bitstream within \a oggz
 * \param name The name of the comment to add
 * \param value The contents of the comment to add
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD \a oggz is not a valid OGGZ* handle
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
int
oggz_comment_add_byname (OGGZ * oggz, long serialno,
                         const char * name, const char * value);

/**
 * Remove a comment
 * \param oggz A OGGZ* handle (created with OGGZ_ENCODE)
 * \param serialno Identify a logical bitstream within \a oggz
 * \param comment The comment to remove.
 * \retval 1 Success: comment removed
 * \retval 0 No-op: comment not found, nothing to remove
 * \retval OGGZ_ERR_BAD \a oggz is not a valid OGGZ* handle
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 */
int
oggz_comment_remove (OGGZ * oggz, long serialno, OggzComment * comment);

/**
 * Remove all comments with a given name.
 * \param oggz A OGGZ* handle (created with OGGZ_ENCODE)
 * \param serialno Identify a logical bitstream within \a oggz
 * \param name The name of the comments to remove
 * \retval ">= 0" The number of comments removed
 * \retval OGGZ_ERR_BAD \a oggz is not a valid OGGZ* handle
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 */
int
oggz_comment_remove_byname (OGGZ * oggz, long serialno, char * name);

/**
 * Output a comment packet for the specified stream
 * \param oggz A OGGZ* handle (created with OGGZ_ENCODE)
 * \param serialno Identify a logical bitstream within \a oggz
 * \returns A comment packet for the stream. The packet and its contents must
 * be freed by the caller.
 * \retval NULL content type does not support comments, not enough memory
 * or comment was too long for FLAC
 * \note In a FLAC comment packet the first bit of the packet data must be
 * set if it is the last header packet. This must be done manually as liboggz
 * cannot tell if there are more header packets to come. E.g.
 * \code packet->packet |= 0x01;
 * \endcode
 */
ogg_packet *
oggz_comment_generate(OGGZ * oggz, long serialno);

#ifdef __cplusplus
}
#endif

#endif /* __OGGZ_COMMENTS_H__ */
