'''
* TO BE DONE:
    -Ensure that audio works correctly.
'''
import ffmpeg
import os

PAD_OPTIONS = {
    'width':'1920',
    'height':'1080',
    'x':'0',
    'y':'0',
	'color': 'black'
}

def main():
    dataDir = 'resources/'
    preFormatVideoDir = 'video_inputs/'
    postFormatVideoDir = 'video_outputs/'
    
    if len(os.listdir(dataDir + preFormatVideoDir)) == 0:
        return
    
    #Converts given videos to mp4 (regardless of format)
    # WORKING
    for i in os.listdir(dataDir + preFormatVideoDir):
        print(i)
        pad_opts = {
            'width':'1920',
            'height':'1080',
            'x':'0',
            'y':'0',
            'color': 'black'
        }
        # Pulling audio and video from input video (ensuring formatting)
        source = dataDir + preFormatVideoDir + i 
        dest = dataDir + postFormatVideoDir + i.split('.')[0] + '.mp4'
        noAudio = True
        info = ffmpeg.probe(source)
        isPicture = 'image' in info['format']['format_name']
        for i in info['streams']:
            if i['codec_type'].lower() == 'audio':
                noAudio = False
            if i['codec_type'].lower() == 'video':
                if i['coded_height'] > i['coded_width']:
                    if i['coded_height'] > int(pad_opts['height']):
                        pad_opts['height'] = str(i['coded_height'])
                    if i['coded_width'] > int(pad_opts['width']):
                        pad_opts['width'] = str(i['coded_width'])
        video_stream = ffmpeg.input(source).video
        video_stream = ffmpeg.filter(video_stream, 'fps', fps=30, round='up')
        video_stream = ffmpeg.filter_(video_stream, 'scale', **{
            'width': '1920',
            'height': '1080'
        })
        audio_stream = ffmpeg.input(source).audio
        stream = None
        if noAudio:
            # getting a null audio source to slap on the video, filter
            # breaks otherwise
            audio_stream = ffmpeg.input('anullsrc', f='lavfi').audio
        if isPicture:
            video_stream = ffmpeg.filter(
                video_stream,
                'fps',
                fps=150,
                round='up'
            )
            stream = ffmpeg.output(
                video_stream,
                audio_stream,
                dest,
                aspect="16:9",
                preset='ultrafast',
                shortest=None,
                r='1'
            )
        else:
            stream = ffmpeg.output(
                video_stream,
                audio_stream,
                dest,
                aspect="16:9",
                preset='ultrafast',
                shortest=None
            )
            
        # Running command
        stream.overwrite_output().global_args(
            '-fflags',
            '+igndts'
        ).global_args(
            '-loglevel',
            'error'
        ).run()

    
    # Generating valid list for ffmpeg to use to concatenate videos
    # WORKING
    files = []
    for i in os.listdir(dataDir + postFormatVideoDir):
        files.append(f"file '{(postFormatVideoDir + i)}'")
    with open(dataDir + 'mp4list', 'w') as f:
        f.write('\n'.join(files))
    
    # Didn't feel like writing this one-liner using ffmpeg-python syntax, this format-string will do    
    # WORKING
    os.system(f"ffmpeg -y -loglevel error -fflags +igndts -f concat -safe 0 -i {dataDir + 'mp4list'} -preset ultrafast {dataDir + 'mergedVid.mp4'}")
    
    
        

if __name__ == '__main__':
    main()
