#!/usr/bin/env bash

am="am_500ms_future_context_dev_other.bin"
lexicon="librispeech-train+dev-unigram-10000-nbest10.lexicon"
token="librispeech-train-all-unigram-10000.tokens"
beamsearch_lexicon="decoder-unigram-10000-nbest10.lexicon"
lm="3-gram.pruned.3e-7.bin.qt"

mkdir -p models/am
cd models/am

if [[ ! -f $am ]] ; then
    wget "https://dl.fbaipublicfiles.com/wav2letter/streaming_convnets/librispeech/models/am/am_500ms_future_context_dev_other.bin"
fi

if [[ ! -f $lexicon ]] ; then
    wget "https://dl.fbaipublicfiles.com/wav2letter/tds/librispeech/librispeech-train%2Bdev-unigram-10000-nbest10.lexicon"
fi

if [[ ! -f $token ]] ; then
    wget "https://dl.fbaipublicfiles.com/wav2letter/streaming_convnets/librispeech/librispeech-train-all-unigram-10000.tokens"
fi

cd ..

mkdir -p decoder
cd decoder

if [[ ! -f $beamsearch_lexicon ]] ; then
    wget "https://dl.fbaipublicfiles.com/wav2letter/streaming_convnets/librispeech/decoder-unigram-10000-nbest10.lexicon"
fi

if [[ ! -f $lm ]] ; then
    wget "https://dl.fbaipublicfiles.com/wav2letter/streaming_convnets/librispeech/models/lm/3-gram.pruned.3e-7.bin.qt"
fi
