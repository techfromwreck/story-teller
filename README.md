# story-teller

Tools used to produce the video ["Words Never Sung" - Youtube](https://www.youtube.com/watch?v=rnjDi1NMVqE).

## What's here

- **`src/story-teller.c`** - reads a text file and "types" it out in the terminal, character by character, for recording.
- **`tools/run.sh`** - builds `story-teller` and runs it against `assets/story.txt`. It only generates the text on screen; recording it as a video is up to you.
- **`tools/generate-waveform.sh`** - renders a waveform video from `assets/example.m4a` using ffmpeg.

## Dependencies

- `gcc` with C2x support (`-std=gnu2x`)
- `ffmpeg`

## Usage

Run from the repository root:

```sh
./tools/run.sh
./tools/generate-waveform.sh
```

## License
MIT license
