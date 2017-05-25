#include "FFmpegAPI.h"

#include <string>

CFFmpegAPI::CFFmpegAPI()
{

}

CFFmpegAPI::~CFFmpegAPI()
{

}

int CFFmpegAPI::MainFunction()
{
    AVFormatContext *m_informat;
    AVFormatContext *m_outformat;
    int ret;
    AVPacket outpkt;
    AVPacket pkt;
    char *errbuf;
    std::string filename = "BF1.mp4";
    int m_in_vid_strm_idx;
    AVStream *m_in_vid_strm;
    AVStream *m_out_vid_strm;
    int m_fps = 30;
    bool m_init_done;
    int m_num_frames = 500;
    //1. Do initialization using 

    av_register_all();

    // 2. Open input file using  

    avformat_open_input(&m_informat, filename.c_str(), 0, 0);

    //3. Find input stream info.

    if ((ret = avformat_find_stream_info(m_informat, 0))< 0)
    {
        av_strerror(ret, errbuf, sizeof(errbuf));
        printf("Not Able to find stream info:: ");
            ret = -1;
        return ret;
    }

    for (unsigned int i = 0; i < m_informat->nb_streams; i++)
    {
        if (m_informat->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("Found Video Stream ");
            m_in_vid_strm_idx = i;
            m_in_vid_strm = m_informat->streams[i];
        }
    }

    // 4. Create ouputfile  and allocate output format.

    AVOutputFormat *outfmt = NULL;
    std::string outfile = std::string(filename) + "clip_out.ts";
    outfmt = av_guess_format(NULL, outfile.c_str(), NULL);
    if (outfmt == NULL)
    {
        ret = -1;
        return ret;
    }
    else
    {
        m_outformat = avformat_alloc_context();
        if (m_outformat)
        {
            m_outformat->oformat = outfmt;
            _snprintf(m_outformat->filename,
                sizeof(m_outformat->filename),
                "%s", outfile.c_str());
        }
        else
        {
            ret = -1;
            return ret;
        }
    }

    //5. Add audio and video stream to output format.

    AVCodec *out_vid_codec, *out_aud_codec;
    out_vid_codec = out_aud_codec = NULL;

    if (outfmt->video_codec != AV_CODEC_ID_NONE && m_in_vid_strm != NULL)
    {
        out_vid_codec = avcodec_find_encoder(outfmt->video_codec);
        if (NULL == out_vid_codec)
        {
            printf("Could Not Find Vid Encoder");
                ret = -1;
            return ret;
        }
        else
        {
            printf("Found Out Vid Encoder ");
                m_out_vid_strm = avformat_new_stream(m_outformat, out_vid_codec);
            if (NULL == m_out_vid_strm)
            {
                printf("Failed to Allocate Output Vid Strm ");
                    ret = -1;
                return ret;
            }
            else
            {
                printf("Allocated Video Stream ");
                    if (avcodec_copy_context(m_out_vid_strm->codec,
                        m_informat->streams[m_in_vid_strm_idx]->codec) != 0)
                    {
                        printf("Failed to Copy Context ");
                        ret = -1;
                        return ret;
                    }
                    else
                    {
                        m_out_vid_strm->sample_aspect_ratio.den = m_out_vid_strm->codec->sample_aspect_ratio.den;
                        m_out_vid_strm->sample_aspect_ratio.num = m_in_vid_strm->codec->sample_aspect_ratio.num;
                        printf("Copied Context ");
                        m_out_vid_strm->codec->codec_id = m_in_vid_strm->codec->codec_id;
                        m_out_vid_strm->codec->time_base.num = 1;
                        m_out_vid_strm->codec->time_base.den = m_fps*(m_in_vid_strm->codec->ticks_per_frame);
                        m_out_vid_strm->time_base.num = 1;
                        m_out_vid_strm->time_base.den = 1000;
                        m_out_vid_strm->r_frame_rate.num = m_fps;
                        m_out_vid_strm->r_frame_rate.den = 1;
                        m_out_vid_strm->avg_frame_rate.den = 1;
                        m_out_vid_strm->avg_frame_rate.num = m_fps;
                        //m_out_vid_strm->duration = (m_out_end_time - m_out_start_time) * 1000;
                    }
            }
        }
    }

    // 6. Finally output header.
    if (!(outfmt->flags & AVFMT_NOFILE))
    {
        if (avio_open2(&m_outformat->pb, outfile.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0)
        {
            printf("Could Not Open File ");
                ret = -1;
            return ret;
        }
    }
    /* Write the stream header, if any. */
    if (avformat_write_header(m_outformat, NULL) < 0)
    {
        printf("Error Occurred While Writing Header ");
            ret = -1;
        return ret;
    }
    else
    {
        printf("Written Output header ");
        m_init_done = true;
    }

    // 7. Now in while loop read frame using av_read_frame and write to output format using 
    //  av_interleaved_write_frame(). You can use following loop

    while (av_read_frame(m_informat, &pkt) >= 0 && (m_num_frames-- > 0))
    {
        if (pkt.stream_index == m_in_vid_strm_idx)
        {
            printf("ACTUAL VID Pkt PTS %d", av_rescale_q(pkt.pts, m_in_vid_strm->time_base, m_in_vid_strm->codec->time_base));
            printf("ACTUAL VID Pkt DTS %d", av_rescale_q(pkt.dts, m_in_vid_strm->time_base, m_in_vid_strm->codec->time_base));
            av_init_packet(&outpkt);
            if (pkt.pts != AV_NOPTS_VALUE)
            {
                //if (last_vid_pts == vid_pts)
                //{
                //    vid_pts++;
                //    last_vid_pts = vid_pts;
                //}
                //outpkt.pts = vid_pts;
                printf("ReScaled VID Pts %d", outpkt.pts);
            }
            else
            {
                outpkt.pts = AV_NOPTS_VALUE;
            }

            if (pkt.dts == AV_NOPTS_VALUE)
            {
                outpkt.dts = AV_NOPTS_VALUE;
            }
            else
            {
                //outpkt.dts = vid_pts;
                printf("ReScaled VID Dts %d", outpkt.dts);
                printf("=======================================");
            }

            outpkt.data = pkt.data;
            outpkt.size = pkt.size;
            outpkt.stream_index = pkt.stream_index;
            outpkt.flags |= AV_PKT_FLAG_KEY;
            //last_vid_pts = vid_pts;
            if (av_interleaved_write_frame(m_outformat, &outpkt) < 0)
            {
                printf("Failed Video Write ");
            }
            else
            {
                m_out_vid_strm->codec->frame_number++;
            }
            av_free_packet(&outpkt);
            av_free_packet(&pkt);
        }
        else
        {
            printf("Got Unknown Pkt ");
                //num_unkwn_pkt++;
        }
        //num_total_pkt++;
    }

    //8. Finally write trailer and clean up everything

    av_write_trailer(m_outformat);
    av_free_packet(&outpkt);
    av_free_packet(&pkt);
}