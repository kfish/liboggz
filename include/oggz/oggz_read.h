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

#ifndef __OGGZ_READ_H__
#define __OGGZ_READ_H__

/** \defgroup read_api OGGZ Read API
 *
 * OGGZ parses Ogg bitstreams, forming ogg_packet structures, and calling
 * your OggzReadPacket callback(s).
 *
 * You provide Ogg data to OGGZ with oggz_read() or oggz_read_input(), and
 * independently process it in OggzReadPacket callbacks.
 * It is possible to set a different callback per \a serialno (ie. for each
 * logical bitstream in the Ogg bitstream - see the \ref basics section for
 * more detail).
 *
 * See \ref seek_api for information on seeking on interleaved Ogg data.
 *
 * \{
 */

/**
 * This is the signature of a callback which you must provide for Oggz
 * to call whenever it finds a new packet in the Ogg stream associated
 * with \a oggz.
 *
 * \param oggz The OGGZ handle
 * \param op The full ogg_packet (see <ogg/ogg.h>)
 * \param serialno Identify the logical bistream in \a oggz that contains
 *                 \a op
 * \param user_data A generic pointer you have provided earlier
 * \returns 0 to continue, non-zero to instruct OGGZ to stop.
 *
 * \note It is possible to provide different callbacks per logical
 * bitstream -- see oggz_set_read_callback() for more information.
 */
typedef int (*OggzReadPacket) (OGGZ * oggz, ogg_packet * op, long serialno,
			       void * user_data);

/**
 * Set a callback for Oggz to call when a new Ogg packet is found in the
 * stream.
 *
 * \param oggz An OGGZ handle previously opened for reading
 * \param serialno Identify the logical bitstream in \a oggz to attach
 * this callback to, or -1 to attach this callback to all unattached
 * logical bitstreams in \a oggz.
 * \param read_packet Your callback function
 * \param user_data Arbitrary data you wish to pass to your callback
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 *
 * \note Values of \a serialno other than -1 allows you to specify different
 * callback functions for each logical bitstream.
 *
 * \note It is safe to call this callback from within an OggzReadPacket
 * function, in order to specify that subsequent packets should be handled
 * by a different OggzReadPacket function.
 */
int oggz_set_read_callback (OGGZ * oggz, long serialno,
			    OggzReadPacket read_packet, void * user_data);

/**
 * Read n bytes into \a oggz, calling any read callbacks on the fly.
 * \param oggz An OGGZ handle previously opened for reading
 * \param n A count of bytes to ingest
 * \retval ">  0" The number of bytes successfully ingested.
 * \retval 0 End of file
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_SYSTEM System error; check errno for details
 * \retval OGGZ_ERR_USER_STOPPED Reading was stopped by a user callback
 * returning OGGZ_STOP_OK or OGGZ_STOP_ERR before any input bytes were
 * consumed. This will occur when a packet is read from a previously
 * buffered page of input data, and stopping is immediately requested.
 */
long oggz_read (OGGZ * oggz, long n);

/**
 * Input data into \a oggz.
 * \param oggz An OGGZ handle previously opened for reading
 * \param buf A memory buffer
 * \param n A count of bytes to input
 * \retval ">  0" The number of bytes successfully ingested.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 * \retval OGGZ_ERR_USER_STOPPED Reading was stopped by a user callback
 * returning OGGZ_STOP_OK or OGGZ_STOP_ERR before any input bytes were
 * consumed. This will occur when a packet is read from a previously
 * buffered page of input data, and stopping is immediately requested.
 */
long oggz_read_input (OGGZ * oggz, unsigned char * buf, long n);

/** \}
 */

#endif /* __OGGZ_READ_H__ */
