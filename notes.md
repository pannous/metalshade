
## 2026-01-26: Shader Download Enhancement
- Enhanced download_shader.sh to accept full ShaderToy URLs (e.g., https://www.shadertoy.com/view/4l2XWK)
- Script automatically extracts shader ID from URLs
- Auto-fetches shader name from API and sanitizes it for filenames
- Consolidated examples/ and shaders/ directories into single shaders/ location
- Updated switch_shader.sh to use new consolidated directory
- Added metalshade user agent for API requests

**Note**: ShaderToy API is protected by CloudFlare which blocks automated downloads. The script works in principle but CloudFlare challenges prevent actual downloads without browser interaction. Users will need to manually copy shader code in most cases.

**Files modified**: download_shader.sh, switch_shader.sh, README.md
**Directories**: Merged examples/ into shaders/

