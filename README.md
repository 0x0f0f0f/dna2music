# dna2music

The `dna2music` reads a FASTA file from standard input and outputs an
intermediate `.txt` file which contains sequences of notes, one by line,
composed by their location, octave, the note played, velocity, duration and
track.

These text files can then converted to MIDI files by the handy
[txt2mid](https://github.com/alambicbicephale/txt2mid) tool, written in C++ by
[Jamil Alioui](https://github.com/alambicbicephale), which I have included in
this repository for simplicity and archival reasons.

## Building

The project only has two dependencies: `g++` and `ocaml` >= 4.03.0. You need to
build `txt2mid` first:

```bash
g++ txt2mid.cpp -o txt2mid
```

## Usage

You can download genomes in FASTA format from the National Institutes of
Health's [GenBank](https://www.ncbi.nlm.nih.gov/genbank/) database,
or from the [UCSC Genome Browser](https://genome.ucsc.edu/).
After obtaining a FASTA file, you can pipe it into `dna2music` and
redirect the output to a text file and then convert the text file to MIDI:

```bash
cat /path/to/file.fasta | ocaml dna2music.ml > file.txt
./txt2mid file.txt
# this will automatically create the MIDI file
```

`dna2music.ml` accepts two additional arguments regarding automatically mapping
the notes to a certain key, to make the generated notes sound more musical
altogether. If no additional argument is supplied key mapping will be skipped.

The first additional argument is the a string containing the key (note) in which
the music will be composed, for example `C` or `E` are valid keys. To use sharp
or flat key use the plus and minus symbols, for example `C+` or `A-`.

The second additional argument is if the scale should be minor or not: `true` is
minor and `false` is major. You can omit this argument to use the major scale.
Other scales are not implemented