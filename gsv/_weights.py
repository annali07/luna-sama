from huggingface_hub import hf_hub_download
from pathlib import Path

REPO_ID  = "nanax14/luna-sama"
REVISION = "v1.0"
FILES    = ["model-1.safetensors", "model-2.safetensors"]

def get_local_weights_dir() -> Path:
    """Always use ./weights inside the package directory."""
    return Path(__file__).resolve().parent / "weights"

def fetch_weights():
    weights_dir = get_local_weights_dir()
    weights_dir.mkdir(exist_ok=True)

    paths = []
    for f in FILES:
        p = hf_hub_download(
            repo_id=REPO_ID,
            filename=f,
            revision=REVISION,
            local_dir=weights_dir,
            local_dir_use_symlinks=False,
            resume_download=True,
        )
        paths.append(Path(p))
    return paths
