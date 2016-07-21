/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/ucontext.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <linux/spi/spidev.h>
#include <execinfo.h>

#if HAVE_PTY_H
#include <pty.h>
#endif

#if HAVE_UTIL_H
#include <util.h>
#endif

/* ------------------------------------------------------------------------- */
/* MARK: Macros and Constants */

#define SPI_HDLC_VERSION                "0.01"

#define MAX_FRAME_SIZE                  2048
#define HEADER_LEN                      5
#define SPI_HEADER_RESET_FLAG			(1 << 7)

#define EXIT_QUIT                       65535

#ifndef FAULT_BACKTRACE_STACK_DEPTH
#define FAULT_BACKTRACE_STACK_DEPTH     20
#endif

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC                    1000
#endif

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC                   1000
#endif

#ifndef USEC_PER_SEC
#define USEC_PER_SEC                    (USEC_PER_MSEC * MSEC_PER_SEC)
#endif

#define SPI_POLL_PERIOD_MSEC            (MSEC_PER_SEC/30)

static const uint8_t kHDLCResetSignal[] = { 0x7E, 0x13, 0x11, 0x7E };
static const uint16_t kHDLCCRCCheckValue = 0x2189;

enum {
    MODE_STDIO = 0,
    MODE_PTY = 1,
};

/* ------------------------------------------------------------------------- */
/* MARK: Global State */

static int gMode = MODE_PTY;
static const char* gSpiDevPath = NULL;
static const char* gIntGpioDevPath = NULL;
static const char* gResGpioDevPath = NULL;
static int gVerbose = LOG_WARNING;

static int gSpiDevFd = -1;
static int gResGpioValueFd = -1;
static int gIntGpioValueFd = -1;

static int gHDLCInputFd = -1;
static int gHDLCOutputFd = -1;

static int gSpiSpeed = 1000000;
static uint8_t gSpiMode = 0;
static int gSpiBeginDelay = 0;

static uint16_t gSpiRxPayloadSize;
static uint8_t gSpiRxFrameBuffer[MAX_FRAME_SIZE];

static uint16_t gSpiTxPayloadSize;
static bool gSpiTxReady = false;
static bool gSpiTxFlowControl = false;
static uint8_t gSpiTxFrameBuffer[MAX_FRAME_SIZE] = { SPI_HEADER_RESET_FLAG };

static bool gSlaveDidReset = false;

static int gRet = 0;

static sig_t gPreviousHandlerForSIGINT;
static sig_t gPreviousHandlerForSIGTERM;

/* ------------------------------------------------------------------------- */
/* MARK: Signal Handlers */

static void signal_SIGINT(int sig)
{
    gRet = EXIT_QUIT;
    syslog(LOG_NOTICE, "Caught SIGINT!");

    // Restore the previous handler so that if we end up getting
    // this signal again we peform the system default action.
    signal(SIGINT, gPreviousHandlerForSIGINT);
    gPreviousHandlerForSIGINT = NULL;
}

static void signal_SIGTERM(int sig)
{
    gRet = EXIT_QUIT;
    syslog(LOG_NOTICE, "Caught SIGTERM!");

    // Restore the previous handler so that if we end up getting
    // this signal again we peform the system default action.
    signal(SIGTERM, gPreviousHandlerForSIGTERM);
    gPreviousHandlerForSIGTERM = NULL;
}

static void signal_SIGHUP(int sig)
{
    gRet = EXIT_FAILURE;
    syslog(LOG_NOTICE, "Caught SIGHUP!");

    // We don't restore the "previous handler"
    // because we always want to let the main
    // loop decide what to do for hangups.
}

static void signal_critical(int sig, siginfo_t * info, void * ucontext)
{
    // This is the last hurah for this process.
    // We dump the stack, because that's all we can do.

    void *stack_mem[FAULT_BACKTRACE_STACK_DEPTH];
    void **stack = stack_mem;
    char **stack_symbols;
    int stack_depth, i;
    ucontext_t *uc = (ucontext_t*)ucontext;

    // Shut up compiler warning.
    (void)uc;

    fprintf(stderr, " *** FATAL ERROR: Caught signal %d (%s):\n", sig, strsignal(sig));

    stack_depth = backtrace(stack, FAULT_BACKTRACE_STACK_DEPTH);

    // Here are are trying to update the pointer in the backtrace
    // to be the actual location of the fault.
#if defined(__x86_64__)
    stack[1] = (void *) uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__i386__)
    stack[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#elif defined(__arm__)
    stack[1] = (void *) uc->uc_mcontext.arm_ip;
#else
#warning TODO: Add this arch to signal_critical
#endif

    // Now dump the symbols to stderr, in case syslog barfs.
    backtrace_symbols_fd(stack, stack_depth, STDERR_FILENO);

    // Load up the symbols individually, so we can output to syslog, too.
    stack_symbols = backtrace_symbols(stack, stack_depth);

    syslog(LOG_CRIT, " *** FATAL ERROR: Caught signal %d (%s):", sig, strsignal(sig));

    for (i = 0; i != stack_depth; i++)
    {
        syslog(LOG_CRIT, "[BT] %2d: %s", i, stack_symbols[i]);
    }

    free(stack_symbols);

    exit(EXIT_FAILURE);
}



/* ------------------------------------------------------------------------- */
/* MARK: SPI Transfer Functions */

static void spi_cs_delay(void)
 {
    struct spi_ioc_transfer xfer = {
        .tx_buf = (unsigned long)NULL,
        .rx_buf = (unsigned long)NULL,
        .len = 0,
        .delay_usecs = gSpiBeginDelay,
        .speed_hz = gSpiSpeed,
        .bits_per_word = 8,
        .cs_change = false,
    };

    // We don't care if this fails.
    ioctl(gSpiDevFd, SPI_IOC_MESSAGE(1), &xfer);
}

static int do_spi_xfer(int len)
 {
    int ret;

    struct spi_ioc_transfer xfer = {
        .tx_buf = (unsigned long)gSpiTxFrameBuffer,
        .rx_buf = (unsigned long)gSpiRxFrameBuffer,
        .len = len + HEADER_LEN,
        .delay_usecs = 0,
        .speed_hz = gSpiSpeed,
        .bits_per_word = 8,
        .cs_change = false,
    };

    if (gSpiBeginDelay > 0)
    {
        spi_cs_delay();
    }

    ret = ioctl(gSpiDevFd, SPI_IOC_MESSAGE(1), &xfer);

    if (gSpiRxFrameBuffer[0] != 0xFF)
    {
        if (gSpiRxFrameBuffer[0] & SPI_HEADER_RESET_FLAG)
        {
            gSlaveDidReset = true;
        }
    }

    return ret;
}


static void spi_header_set_accept_len(uint8_t *header, uint16_t len)
{
    header[1] = ((len << 0) & 0xFF);
    header[2] = ((len << 8) & 0xFF);
}

static void spi_header_set_data_len(uint8_t *header, uint16_t len)
{
    header[3] = ((len << 0) & 0xFF);
    header[4] = ((len << 8) & 0xFF);
}

static uint16_t spi_header_get_accept_len(uint8_t *header)
{
    return ( header[1] + (header[2] << 8) );
}

static uint16_t spi_header_get_data_len(uint8_t *header)
{
    return ( header[3] + (header[4] << 8) );
}


static int push_pull_spi()
 {
    int ret;
    uint16_t slave_max_rx;
    uint16_t slave_data_len;
    int spi_xfer_bytes = 0;

    gSpiTxFlowControl = false;

    // Fetch the slave's buffer sizes.
    // Zero out our max rx and data len
    // so that the slave doesn't think
    // we are actually trying to transfer
    // data.
    spi_header_set_accept_len(gSpiTxFrameBuffer, 0);
    spi_header_set_data_len(gSpiTxFrameBuffer, 0);
    ret = do_spi_xfer(0);
    if (ret < 0)
    {
        perror("do_spi_xfer");
        goto bail;
    }

    if (gSpiRxFrameBuffer[0] == 0xFF)
    {
        // Device is off or in a bad state.
        gSpiTxFlowControl = true;

        syslog(LOG_DEBUG, "Discarded frame. (1)");
        goto bail;
    }

    slave_max_rx = spi_header_get_accept_len(gSpiRxFrameBuffer);
    slave_data_len = spi_header_get_data_len(gSpiRxFrameBuffer);

    if ( (slave_max_rx > MAX_FRAME_SIZE)
      || (slave_data_len > MAX_FRAME_SIZE)
    )
    {
        gSpiTxFlowControl = true;
        syslog(
            LOG_INFO,
            "Gibberish in header (max_rx:%d, data_len:%d)",
            slave_max_rx,
            slave_data_len
        );
        goto bail;
    }

    if ( (gSpiTxReady != 0)
      && (gSpiTxPayloadSize <= slave_max_rx)
    )
    {
        spi_xfer_bytes = gSpiTxPayloadSize;
        spi_header_set_data_len(gSpiTxFrameBuffer, gSpiTxPayloadSize);
    }
    else if (gSpiTxReady && (gSpiTxPayloadSize > slave_max_rx))
    {
        // The slave isn't ready for what we have to
        // send them. Turn on rate limiting so that we
        // don't waste a ton of CPU bombarding them
        // with useless SPI transfers.
        gSpiTxFlowControl = true;
    }

    if ( (slave_data_len != 0)
      && (gSpiRxPayloadSize == 0)
    )
    {
        spi_header_set_accept_len(gSpiTxFrameBuffer, slave_data_len);
        if (slave_data_len > spi_xfer_bytes)
        {
            spi_xfer_bytes = slave_data_len;
        }
    }

    // This is the real transfer.
    ret = do_spi_xfer(spi_xfer_bytes);
    if (ret < 0)
    {
        perror("do_spi_xfer");
        goto bail;
    }

    if (gSpiRxFrameBuffer[0] == 0xFF)
    {
        // Device is off or in a bad state.
        gSpiTxFlowControl = true;

        syslog(LOG_DEBUG, "Discarded frame. (2)");
        goto bail;
    }

    slave_max_rx = spi_header_get_accept_len(gSpiRxFrameBuffer);
    slave_data_len = spi_header_get_data_len(gSpiRxFrameBuffer);

    if ( (gSpiRxPayloadSize == 0)
      && (slave_data_len <= spi_header_get_accept_len(gSpiTxFrameBuffer))
    ) {
        // We have received a packet. Set gSpiRxPayloadSize so that
        // the packet will eventually get queued up by push_hdlc().
        gSpiRxPayloadSize = slave_data_len;
    }

    if ( (gSpiTxPayloadSize == spi_header_get_data_len(gSpiTxFrameBuffer))
      && (spi_header_get_data_len(gSpiTxFrameBuffer) <= slave_max_rx)
    ) {
        // Out outbound packet has been successfully transmitted. Clear
        // gSpiTxPayloadSize and gSpiTxReady so that pull_hdlc() can
        // pull another packet for us to send.
        gSpiTxReady = false;
        gSpiTxPayloadSize = 0;
    }

bail:
    return ret;
}

static bool check_and_clear_interrupt(void)
{
    char value[5] = "";
    int len;

    lseek(gIntGpioValueFd, 0, SEEK_SET);

    len = read(gIntGpioValueFd, value, sizeof(value)-1);

    if (len < 0)
    {
        perror("check_and_clear_interrupt");
        gRet = EXIT_FAILURE;
    }

    return 0 != atoi(value);
}

/* ------------------------------------------------------------------------- */
/* MARK: HDLC Transfer Functions */

#define HDLC_BYTE_FLAG             0x7E
#define HDLC_BYTE_ESC              0x7D
#define HDLC_BYTE_XON              0x11
#define HDLC_BYTE_XOFF             0x13
#define HDLC_BYTE_SPECIAL          0xF8
#define HDLC_ESCAPE_XFORM          0x20

static uint16_t hdlc_crc16(uint16_t aFcs, uint8_t aByte)
{
#if 1
    // CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-CCITT
    // width=16 poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000 check=0x2189 name="KERMIT"
    // http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.kermit
    static const uint16_t sFcsTable[256] =
    {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
    };
    return (aFcs >> 8) ^ sFcsTable[(aFcs ^ aByte) & 0xff];
#else
    // CRC-16/CCITT-FALSE, same CRC as 802.15.4
    // width=16 poly=0x1021 init=0xffff refin=false refout=false xorout=0x0000 check=0x29b1 name="CRC-16/CCITT-FALSE"
    // http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.crc-16-ccitt-false
    aFcs = (uint16_t)((aFcs >> 8) | (aFcs << 8));
    aFcs ^= aByte;
    aFcs ^= ((aFcs & 0xff) >> 4);
    aFcs ^= (aFcs << 12);
    aFcs ^= ((aFcs & 0xff) << 5);
    return aFcs;
#endif
}

static bool hdlc_byte_needs_escape(uint8_t byte)
{
    switch(byte)
    {
    case HDLC_BYTE_SPECIAL:
    case HDLC_BYTE_ESC:
    case HDLC_BYTE_FLAG:
    case HDLC_BYTE_XOFF:
    case HDLC_BYTE_XON:
        return true;

    default:
        return false;
    }
}

static int push_hdlc(void)
{
    int ret = 0;
    static uint8_t escaped_frame_buffer[MAX_FRAME_SIZE*2];
    static uint16_t escaped_frame_len;
    static uint16_t escaped_frame_sent;

    if (escaped_frame_len == 0)
    {
        if (gSlaveDidReset)
        {
            // Indicate an MCU reset.
            memcpy(escaped_frame_buffer, kHDLCResetSignal, sizeof(kHDLCResetSignal));
            escaped_frame_len = sizeof(kHDLCResetSignal);
            gSlaveDidReset = false;

        }
        else if (gSpiRxPayloadSize != 0)
        {
            // Escape the frame.
            uint8_t c;
            uint16_t fcs = 0x0000;
            uint16_t i;

            for (i = 0; i < gSpiRxPayloadSize; i++)
            {
                c = gSpiRxFrameBuffer[i];
                fcs = hdlc_crc16(fcs, c);
                if (hdlc_byte_needs_escape(c))
                {
                    escaped_frame_buffer[escaped_frame_len++] = HDLC_BYTE_ESC;
                    escaped_frame_buffer[escaped_frame_len++] = c ^ HDLC_ESCAPE_XFORM;
                }
                else
                {
                    escaped_frame_buffer[escaped_frame_len++] = c;
                }
            }

            fcs ^= 0xFFFF;

            c = fcs & 0xFF;
            if (hdlc_byte_needs_escape(c))
            {
                escaped_frame_buffer[escaped_frame_len++] = HDLC_BYTE_ESC;
                escaped_frame_buffer[escaped_frame_len++] = c ^ HDLC_ESCAPE_XFORM;
            }
            else
            {
                escaped_frame_buffer[escaped_frame_len++] = c;
            }

            c = (fcs >> 8) & 0xFF;
            if (hdlc_byte_needs_escape(c))
            {
                escaped_frame_buffer[escaped_frame_len++] = HDLC_BYTE_ESC;
                escaped_frame_buffer[escaped_frame_len++] = c ^ HDLC_ESCAPE_XFORM;
            }
            else
            {
                escaped_frame_buffer[escaped_frame_len++] = c;
            }

            escaped_frame_buffer[escaped_frame_len++] = HDLC_BYTE_FLAG;
            escaped_frame_sent = 0;
            gSpiRxPayloadSize = 0;

        }
        else
        {
            // Nothing to do.
            goto bail;
        }
    }

    ret = write(
        gHDLCOutputFd,
        escaped_frame_buffer + escaped_frame_sent,
        escaped_frame_len    - escaped_frame_sent
    );

    if (ret < 0)
    {
        if (errno == EAGAIN)
        {
            ret = 0;
        }
        else
        {
            perror("push_hdlc:write");
            syslog(LOG_ERR, "push_hdlc:write: errno=%d (%s)", errno, strerror(errno));
        }
        goto bail;
    }

    escaped_frame_sent += ret;

    // Reset state once we have sent the entire frame.
    if (escaped_frame_len == escaped_frame_sent)
    {
        escaped_frame_len = escaped_frame_sent = 0;
    }

    ret = 0;

bail:
    return ret;
}

static int pull_hdlc(void)
{
    int ret = 0;
    static uint16_t fcs;
    static bool unescape_next_byte = false;

    if (!gSpiTxReady)
    {
        uint8_t byte;
        while ((ret = read(gHDLCInputFd, &byte, 1)) == 1)
        {
            if (gSpiTxPayloadSize >= (MAX_FRAME_SIZE - HEADER_LEN))
            {
                syslog(LOG_WARNING, "HDLC frame was too big");
                unescape_next_byte = false;
                gSpiTxPayloadSize = 0;
                fcs = 0;

            }
            else if (byte == HDLC_BYTE_FLAG)
            {
                if (gSpiTxPayloadSize <= 2)
                {
                    unescape_next_byte = false;
                    gSpiTxPayloadSize = 0;
                    fcs = 0;
                    continue;

                }
                else if (fcs != kHDLCCRCCheckValue)
                {
                    syslog(LOG_WARNING, "HDLC frame with bad CRC");
                    unescape_next_byte = false;
                    gSpiTxPayloadSize = 0;
                    fcs = 0;
                    continue;
                }

                // Clip off the CRC
                gSpiTxPayloadSize -= 2;

                // Indicate that a frame is ready to go out
                gSpiTxReady = true;

                // Clean up for the next frame
                unescape_next_byte = false;
                fcs = 0;
                break;

            }
            else if (byte == HDLC_BYTE_ESC)
            {
                unescape_next_byte = true;
                continue;

            }
            else if (hdlc_byte_needs_escape(byte))
            {
                // Skip all other control codes.
                continue;

            }
            else if (unescape_next_byte)
            {
                byte = byte ^ HDLC_ESCAPE_XFORM;
            }

            fcs = hdlc_crc16(fcs, byte);
            gSpiTxFrameBuffer[HEADER_LEN + gSpiTxPayloadSize++] = byte;
        }
    }

    if (ret < 0)
    {
        if (errno == EAGAIN)
        {
            ret = 0;
        }
        else
        {
            perror("pull_hdlc:read");
            syslog(LOG_ERR, "pull_hdlc:read: errno=%d (%s)", errno, strerror(errno));
        }
    }

    return ret < 0
        ? ret
        : 0;
}


/* ------------------------------------------------------------------------- */
/* MARK: Setup Functions */

static bool update_spi_mode(int x)
{
    gSpiMode = (uint8_t)x;

    if ( (gSpiDevFd >= 0)
      && (ioctl(gSpiDevFd, SPI_IOC_WR_MODE, &gSpiMode) < 0)
    )
    {
        perror("ioctl(SPI_IOC_WR_MODE)");
        return false;
    }

    return true;
}

static bool update_spi_speed(int x)
{
    gSpiSpeed = x;

    if ( (gSpiDevFd >= 0)
      && (ioctl(gSpiDevFd, SPI_IOC_WR_MAX_SPEED_HZ, &gSpiSpeed) < 0)
    )
    {
        perror("ioctl(SPI_IOC_WR_MAX_SPEED_HZ)");
        return false;
    }

    return true;
}


static bool setup_spi_dev(const char* path)
{
    int fd = -1;
    const uint8_t spi_word_bits = 8;
    int ret;
    gSpiDevPath = path;

    fd = open(path, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        goto bail;
    }

    // Set the SPI mode.
    ret = ioctl(fd, SPI_IOC_WR_MODE, &gSpiMode);
    if (ret < 0)
    {
        perror("ioctl(SPI_IOC_WR_MODE)");
        goto bail;
    }

    // Set the SPI clock speed.
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &gSpiSpeed);
    if (ret < 0)
    {
        perror("ioctl(SPI_IOC_WR_MAX_SPEED_HZ)");
        goto bail;
    }

    // Set the SPI word size.
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_word_bits);
    if (ret < 0)
    {
        perror("ioctl(SPI_IOC_WR_BITS_PER_WORD)");
        goto bail;
    }

    // Lock the file descriptor
    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
    {
        perror("flock");
        goto bail;
    }

    gSpiDevFd = fd;
    fd = -1;

bail:
    if (fd >= 0)
    {
        close(fd);
    }
    return gSpiDevFd >= 0;
}

static bool setup_res_gpio(const char* path)
{
    int setup_fd;
    int prev_errno = 0;
    char* dir_path = NULL;
    char* value_path = NULL;
    int len;

    gResGpioDevPath = path;

    len = asprintf(&dir_path, "%s/direction", path);

    if (len < 0)
    {
        perror("asprintf");
        goto bail;
    }

    len = asprintf(&value_path, "%s/value", path);

    if (len < 0)
    {
        perror("asprintf");
        goto bail;
    }

    setup_fd = open(dir_path, O_WRONLY);

    if (setup_fd >= 0)
    {
        if (-1 == write(setup_fd, "high\n", 5))
        {
            perror("set_res_direction");
            goto bail;
        }
    }

    gResGpioValueFd = open(value_path, O_WRONLY);

bail:

    if (setup_fd >= 0)
    {
        close(setup_fd);
    }

    if (dir_path)
    {
        free(dir_path);
    }

    if (value_path)
    {
        free(value_path);
    }

    return gResGpioValueFd >= 0;
}

static void trigger_reset(void)
{
    if (gResGpioValueFd >= 0)
    {
        lseek(gResGpioValueFd, 0, SEEK_SET);
        write(gResGpioValueFd, "0\n", 2);

        usleep(10 * USEC_PER_MSEC);

        lseek(gResGpioValueFd, 0, SEEK_SET);
        write(gResGpioValueFd, "1\n", 2);

        syslog(LOG_NOTICE, "Triggered hardware reset");
    }
}

static bool setup_int_gpio(const char* path)
{
    char* edge_path = NULL;
    char* dir_path = NULL;
    char* value_path = NULL;
    int len;
    int setup_fd = -1;

    gIntGpioValueFd = -1;

    gIntGpioDevPath = path;

    len = asprintf(&dir_path, "%s/direction", path);

    if (len < 0)
    {
        perror("asprintf");
        goto bail;
    }

    len = asprintf(&edge_path, "%s/edge", path);

    if (len < 0)
    {
        perror("asprintf");
        goto bail;
    }

    len = asprintf(&value_path, "%s/value", path);

    if (len < 0)
    {
        perror("asprintf");
        goto bail;
    }

    setup_fd = open(dir_path, O_WRONLY);

    if (setup_fd >= 0)
    {
        len = write(setup_fd, "in", 2);
        if (len < 0)
        {
            perror("write");
            goto bail;
        }

        close(setup_fd);

        setup_fd = -1;
    }

    setup_fd = open(edge_path, O_WRONLY);

    if (setup_fd >= 0)
    {
        len = write(setup_fd, "falling", 7);

        if (len < 0)
        {
            perror("write");
            goto bail;
        }

        close(setup_fd);

        setup_fd = -1;
    }

    gIntGpioValueFd = open(value_path, O_RDONLY);

bail:

    if (setup_fd >= 0)
    {
        close(setup_fd);
    }

    if (edge_path)
    {
        free(edge_path);
    }

    if (dir_path)
    {
        free(dir_path);
    }

    if (value_path)
    {
        free(value_path);
    }

    return gIntGpioValueFd >= 0;
}


/* ------------------------------------------------------------------------- */
/* MARK: Help */

static void print_version(void)
{
    printf("spi-hdlc " SPI_HDLC_VERSION "(" __TIME__ " " __DATE__ ")\n");
    printf("Copyright (c) 2016 Nest Labs, All Rights Reserved\n");
}

static void print_help(void)
{
    print_version();
    const char* help =
    "\n"
    "Syntax:\n"
    "\n"
    "    spi-hdlc [options] <spi-device-path>\n"
    "\n"
    "Options:\n"
    "\n"
    "    --stdio ...................... Use `stdin` and `stdout` for HDLC input and\n"
    "                                   output. Useful when directly started by the\n"
    "                                   program that will be using it.\n"
#if HAVE_OPENPTY
    "    --pty ........................ Create a pseudoterminal for HDLC input and\n"
    "                                   output. The path of the newly-created PTY\n"
    "                                   will be written to `stdout`, followed by a\n"
    "                                   newline.\n"
#endif // HAVE_OPENPTY
    "    -i/--gpio-int[=gpio-path] .... Specify a path to the Linux sysfs-exported\n"
    "                                   GPIO directory for the `I̅N̅T̅` pin. If not\n"
    "                                   specified, `spi-hdlc` will fall back to\n"
    "                                   polling, which is inefficient.\n"
    "    -r/--gpio-reset[=gpio-path] .. Specify a path to the Linux sysfs-exported\n"
    "                                   GPIO directory for the `R̅E̅S̅` pin.\n"
    "    --spi-mode[=mode] ............ Specify the SPI mode to use (0-3).\n"
    "    --spi-speed[=hertz] .......... Specify the SPI speed in hertz.\n"
    "    -v/--verbose ................. Increase debug verbosity. (Repeatable)\n"
    "    -h/-?/--help ................. Print out usage information and exit.\n"
    "\n";

    printf("%s", help);
}


/* ------------------------------------------------------------------------- */
/* MARK: Main Loop */

int main(int argc, char *argv[])
{
    int i = 0;
    const char* prog = argv[0];
    struct sigaction sigact;
    static fd_set read_set;
    static fd_set write_set;
    static fd_set error_set;
    struct timeval timeout;
    int max_fd = -1;
    enum {
        ARG_SPI_MODE = 1001,
        ARG_SPI_SPEED = 1002
    };

    static struct option options[] = {
        { "stdio",      no_argument,       &gMode, MODE_STDIO    },
        { "pty",        no_argument,       &gMode, MODE_PTY      },
        { "gpio-int",   required_argument, NULL,   'i'           },
        { "gpio-res",   required_argument, NULL,   'r'           },
        { "verbose",    optional_argument, NULL,   'v'           },
        { "version",    no_argument,       NULL,   'V'           },
        { "help",       no_argument,       NULL,   'h'           },
        { "spi-mode",   required_argument, NULL,   ARG_SPI_MODE  },
        { "spi-speed",  required_argument, NULL,   ARG_SPI_SPEED },

        { NULL,         0,                 NULL,   0             },
    };

    if (argc < 2)
    {
        print_help();
        exit(EXIT_FAILURE);
    }


    // ========================================================================
    // INITIALIZATION

    gPreviousHandlerForSIGINT = signal(SIGINT, &signal_SIGINT);
    gPreviousHandlerForSIGTERM = signal(SIGTERM, &signal_SIGTERM);
    signal(SIGHUP, &signal_SIGHUP);

    sigact.sa_sigaction = &signal_critical;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO | SA_NOCLDWAIT;

    sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL);
    sigaction(SIGBUS, &sigact, (struct sigaction *)NULL);
    sigaction(SIGILL, &sigact, (struct sigaction *)NULL);
    sigaction(SIGABRT, &sigact, (struct sigaction *)NULL);


    // ========================================================================
    // ARGUMENT PARSING

    openlog(basename(prog), LOG_PERROR | LOG_PID | LOG_CONS, LOG_DAEMON);

    setlogmask(setlogmask(0) & LOG_UPTO(gVerbose));

    while (1)
    {
        int c = getopt_long(argc, argv, "i:r:v:Vh?", options, NULL);
        if (c == -1)
        {
            break;
        }
        else
        {
            switch (c)
            {
            case 'i':
                if (!setup_int_gpio(optarg))
                {
                    syslog(LOG_ERR, "Unable to setup INT GPIO \"%s\", %s", optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;

            case ARG_SPI_MODE:
                if (!update_spi_mode(atoi(optarg)))
                {
                    syslog(LOG_ERR, "Unable to set SPI mode to \"%s\", %s", optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;

            case ARG_SPI_SPEED:
                if (!update_spi_speed(atoi(optarg)))
                {
                    syslog(LOG_ERR, "Unable to set SPI speed to \"%s\", %s", optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;

            case 'r':
                if (!setup_res_gpio(optarg))
                {
                    syslog(LOG_ERR, "Unable to setup RES GPIO \"%s\", %s", optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;

            case 'v':
                if (gVerbose < LOG_DEBUG)
                {
                    if (optarg)
                    {
                        gVerbose += atoi(optarg);
                    }
                    else
                    {
                        gVerbose++;
                    }
                    setlogmask(setlogmask(0) & LOG_UPTO(gVerbose));
                }
                break;

            case 'V':
                print_version();
                exit(EXIT_SUCCESS);
                break;

            case 'h':
            case '?':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            }
        }
    }

    argc -= optind;
    argv += optind;

    if (argc >= 1)
    {
        if (!setup_spi_dev(argv[0]))
        {
            syslog(LOG_ERR, "%s: Unable to open SPI device \"%s\", %s", prog, argv[optind], strerror(errno));
            exit(EXIT_FAILURE);
        }
        argc--;
        argv++;
    }

    if (argc >= 1)
    {
        fprintf(stderr, "%s: Unexpected argument \"%s\"\n", prog, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (gSpiDevPath == NULL)
    {
        fprintf(stderr, "%s: Missing SPI device path\n", prog);
        exit(EXIT_FAILURE);
    }

    if (gMode == MODE_STDIO)
    {
        gHDLCInputFd = dup(STDIN_FILENO);
        gHDLCOutputFd = dup(STDOUT_FILENO);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

    }
    else if (gMode == MODE_PTY)
    {
#if HAVE_OPENPTY

        static int pty_slave_fd = -1;
        char pty_name[1024];
        gRet = openpty(&gHDLCInputFd, &pty_slave_fd, pty_name, NULL, NULL);

        if (gRet != 0)
        {
            perror("openpty");
            goto bail;
        }

        gHDLCOutputFd = dup(gHDLCInputFd);

        printf("%s\n", pty_name);

        close(STDOUT_FILENO);

#else // if HAVE_OPENPTY

        syslog(LOG_ERR, "Not built with support for `--pty`.");
        gRet = EXIT_FAILURE;
        goto bail;

#endif // else HAVE_OPENPTY

    }
    else
    {
        gRet = EXIT_FAILURE;
        goto bail;
    }

    // Set up gHDLCInputFd for non-blocking I/O
    if (-1 == (i = fcntl(gHDLCInputFd, F_GETFL, 0)))
    {
        i = 0;
    }
    fcntl(gHDLCInputFd, F_SETFL, i | O_NONBLOCK);

    // Since there are so few file descriptors in
    // this program, we calcualte `max_fd` once
    // instead of trying to optimize its value
    // at every iteration.
    max_fd = gHDLCInputFd;

    if (max_fd < gHDLCOutputFd)
    {
        max_fd = gHDLCOutputFd;
    }

    if (max_fd < gIntGpioValueFd)
    {
        max_fd = gIntGpioValueFd;
    }

    if (gIntGpioValueFd < 0)
    {
        syslog(LOG_WARNING, "Interrupt pin was not set, must poll SPI. Performance will suffer.");
    }

    trigger_reset();

    // ========================================================================
    // MAIN LOOP

    while (gRet == 0)
    {
        int timeout_ms = 60 * MSEC_PER_SEC;

        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&error_set);

        if (!gSpiTxReady)
        {
            FD_SET(gHDLCInputFd, &read_set);

        }
        else if (gSpiTxFlowControl)
        {
            // We are being rate-limited by the NCP.
            timeout_ms = SPI_POLL_PERIOD_MSEC;
            syslog(LOG_INFO, "Rate limiting transactions");
        }
        else
        {
            // We have data to send to the slave. Unless we
            // are being rate-limited, proceed immediately.
            timeout_ms = 0;
        }

        if (gSpiRxPayloadSize != 0)
        {
            FD_SET(gHDLCOutputFd, &write_set);

        }
        else if (gIntGpioValueFd >= 0)
        {
            if (check_and_clear_interrupt())
            {
                // Interrupt pin is asserted,
                // set the timeout to be 0.
                timeout_ms = 0;

                syslog(LOG_DEBUG, "Interrupt.");
            }
            else
            {
                FD_SET(gIntGpioValueFd, &error_set);
            }

        }
        else if (timeout_ms > SPI_POLL_PERIOD_MSEC)
        {
            timeout_ms = SPI_POLL_PERIOD_MSEC;
        }

        // Calculate the timeout value.
        timeout.tv_sec = timeout_ms / MSEC_PER_SEC;
        timeout.tv_usec = (timeout_ms % MSEC_PER_SEC) * USEC_PER_MSEC;

        // Wait for something to happen.
        i = select(max_fd + 1, &read_set, &write_set, &error_set, &timeout);

        // Handle serial input.
        if (FD_ISSET(gHDLCInputFd, &read_set))
        {
            // Read in the data.
            if (pull_hdlc() < 0)
            {
                gRet = EXIT_FAILURE;
                break;
            }
        }

        // Handle serial output.
        if (FD_ISSET(gHDLCOutputFd, &write_set))
        {
            // Write out the data.
            if (push_hdlc() < 0)
            {
                gRet = EXIT_FAILURE;
                break;
            }
        }

        // Service the SPI port if we can receive
        // a packet or we have a packet to be sent.
        if ((gSpiRxPayloadSize == 0) || gSpiTxReady)
        {
            if (push_pull_spi() < 0)
            {
                gRet = EXIT_FAILURE;
            }
        }
    }


    // ========================================================================
    // SHUTDOWN

bail:
    syslog(LOG_NOTICE, "Shutdown. (gRet = %d)", gRet);

    if (gRet == EXIT_QUIT)
    {
        gRet = EXIT_SUCCESS;
    }
    else if (gRet == -1)
    {
        gRet = EXIT_FAILURE;
    }

    return gRet;
}
