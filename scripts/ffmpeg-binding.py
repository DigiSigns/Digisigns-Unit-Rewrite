'''
* TO BE DONE:
    -Ensure that audio works correctly.
'''
import ffmpeg
from multiprocessing import Pool
import os

dataDir = 'resources/'
preFormatVideoDir = 'video_inputs/'
postFormatVideoDir = 'video_outputs/'
preDirName = dataDir + preFormatVideoDir
postDirName = dataDir + postFormatVideoDir

# Converts one video to mp4 with standard parameters
def processVideo(fileName):
    source = preDirName + fileName
    dest = postDirName + fileName.split('.')[0] + '.mp4'
    video_stream = ffmpeg.input(source).video
    video_stream = ffmpeg.filter(video_stream, 'fps', fps=25, round='up')
    audio_stream = ffmpeg.input(source).audio
    # Setting up output streams with correct destination
    stream = ffmpeg.output(video_stream, audio_stream, dest)
    # Running command
    try:
        stream.overwrite_output().run()
    except:
        pass

def main():
    dirList = os.listdir(preDirName)
    if len(dirList) == 0:
        print('sucks')
        return

    with Pool(8) as p:
        p.map(processVideo, dirList)
    
    # Generating valid list for ffmpeg to use to concatenate videos
    # WORKING
    files = []
    postDirList = os.listdir(postDirName)
    for i in postDirList:
        files.append(f"file '{(postFormatVideoDir + i)}'")
    with open(dataDir + 'mp4list', 'w') as f:
        f.write('\n'.join(files))
    
    # Didn't feel like writing this one-liner using ffmpeg-python syntax, this format-string will do    
    # WORKING
    os.system(
        f"ffmpeg -y -fflags +igndts -f concat -safe 0 -i {dataDir + 'mp4list'} {dataDir + 'mergedVid.mp4'}"
    )

if __name__ == '__main__':
    main()
