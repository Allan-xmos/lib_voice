import os
from pathlib import Path
import numpy as np
import soundfile as sf
import scipy.signal as spsig

from audio_generation import get_band_limited_noise

SAMPLE_RATE = 16000

def make_impulse(RT, t=None, fs=None):
    scale = 0.005
    scale_noise = 0.00005
    a = 3.0 * np.log(10.0) / RT
    if t is None:
        t = np.arange(2.0 * RT * fs) / fs
    N = t.shape[0]
    h = np.zeros(N)
    e = np.exp(-a * t)
    reflections = N // 100
    reflection_index = np.random.randint(N, size=reflections)
    for n, idx in enumerate(reflection_index):
        flip = 1 if (n % 2 == 0) else -1
        h[idx] = flip * scale * t[idx] * e[idx]
    h += scale_noise * np.random.randn(t.shape[0]) * e
    return h


def generate_ns_test_audio(frame_advance):
    """
    Input generator function for `ns` module
    """
    max_freq = SAMPLE_RATE // 2
    db = -20
    sample_count = frame_advance * 50
    return get_band_limited_noise(0, max_freq, samples=sample_count, db=db, sample_rate=SAMPLE_RATE)


def generate_agc_test_audio(frame_advance):
    """
    Input generator function for `agc` module
    """
    max_freq = SAMPLE_RATE // 2
    db = -20
    sample_count = frame_advance * 50
    return get_band_limited_noise(0, max_freq, samples=sample_count, db=db, sample_rate=SAMPLE_RATE)


def generate_ic_test_audio(frame_advance):
    """
    Input generator function for `ic` module
    """
    sample_count = SAMPLE_RATE * 10
    np.random.seed(500)

    phases = 10
    fN = phases * frame_advance

    RT = 0.15
    h = make_impulse(RT, fs=SAMPLE_RATE)
    h = h / h.max()
    hN = len(h)

    u = np.random.randn(sample_count)
    d = spsig.convolve(u, h, "full")[:sample_count]
    if fN > hN:
        d = d[hN - 1 : hN - fN]
    else:
        d = d[hN - 1 :]

    sig_level = 0.01
    d = d * sig_level
    u = u * sig_level

    in_data = np.stack((d, u[hN - 1 : sample_count]), axis=0)
    # Crop to full frames
    inx = (in_data.shape[1] // frame_advance) * frame_advance
    return in_data[:, :inx]


def generate_vnr_test_audio(frame_advance):
    """
    Input generator function for `vnr` module
    """
    hydra_audio_path = Path(os.environ.get("hydra_audio_PATH", "~/hydra_audio")).expanduser()
    wav_name = hydra_audio_path / "vnr_profile_test_stream" / "vnr_profile_stream.wav"
    input_data, _ = sf.read(wav_name)
    return input_data


def generate_aec_test_audio(frame_advance):
    """
    Input generator function for `aec` module
    """
    hydra_audio_path = Path(os.environ.get("hydra_audio_PATH", "~/hydra_audio")).expanduser()
    wav_name = hydra_audio_path / "adec_profile_test_stream" / "input_aec_delay_change_short.wav"
    input_data, _ = sf.read(wav_name)
    return input_data.T


def generate_adec_test_audio(frame_advance):
    """
    Input generator function for `adec` module
    """
    return generate_aec_test_audio(frame_advance) # same input as AEC
