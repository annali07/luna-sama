import torch
import torchaudio
from .module.mel_processing import mel_spectrogram_torch, spectrogram_torch

# cache resamplers per (sr0,sr1,device)
_RESAMPLERS: dict[str, torchaudio.transforms.Resample] = {}

def resample(audio_tensor: torch.Tensor, sr0: int, sr1: int, device: torch.device) -> torch.Tensor:
    key = f"{sr0}->{sr1}@{device}"
    r = _RESAMPLERS.get(key)
    if r is None:
        r = torchaudio.transforms.Resample(sr0, sr1).to(device)
        _RESAMPLERS[key] = r
    return r(audio_tensor)

def get_spepc(hps, filename: str, dtype, device, is_v2pro: bool=False):
    """Load wav -> mono -> target SR -> spectrogram (+16k audio for v2Pro)."""
    sr1 = int(hps.data.sampling_rate)
    audio, sr0 = torchaudio.load(filename)
    audio = audio.to(device)
    if audio.shape[0] == 2:
        audio = audio.mean(0, keepdim=True)
    if sr0 != sr1:
        audio = resample(audio, sr0, sr1, device)

    maxx = audio.abs().max()
    if maxx > 1:
        audio /= min(2, maxx)

    spec = spectrogram_torch(
        audio,
        hps.data.filter_length,
        hps.data.sampling_rate,
        hps.data.hop_length,
        hps.data.win_length,
        center=False,
    ).to(dtype)

    if is_v2pro:
        audio16k = resample(audio, sr1, 16000, device).to(dtype)
        return spec, audio16k
    return spec, audio

# --- v3/v4 mel helpers ---
_SPEC_MIN = -12.
_SPEC_MAX = 2.

def norm_spec(x: torch.Tensor) -> torch.Tensor:
    return (x - _SPEC_MIN) / (_SPEC_MAX - _SPEC_MIN) * 2 - 1

def denorm_spec(x: torch.Tensor) -> torch.Tensor:
    return (x + 1) / 2 * (_SPEC_MAX - _SPEC_MIN) + _SPEC_MIN

def mel_fn_v3(x: torch.Tensor) -> torch.Tensor:
    return mel_spectrogram_torch(
        x, n_fft=1024, win_size=1024, hop_size=256,
        num_mels=100, sampling_rate=24000, fmin=0, fmax=None, center=False
    )

def mel_fn_v4(x: torch.Tensor) -> torch.Tensor:
    return mel_spectrogram_torch(
        x, n_fft=1280, win_size=1280, hop_size=320,
        num_mels=100, sampling_rate=32000, fmin=0, fmax=None, center=False
    )
