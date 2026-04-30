## 1. Các nguyên tắc cần tuân thủ

**Nguyên tắc 1 — Loại bỏ tất cả logic install/verify:**
- `build.ps1`: comment `# (Removed all package manager wrappers and install logic)`, không còn `Install-WingetPackagesIfMissing`, `Install-ChocoPackagesIfMissing`
- `build.sh`: comment `# (Removed all package manager wrappers and install logic)`, các function gọi `install_apt_packages_if_missing libsdl3-dev` đã bị **comment out** ngay tại chỗ:
  ```bash
  # Removed install_apt_packages_if_missing libsdl3-dev (no package install in CI)
  ```
- `deploy-pages.yml`: hoàn toàn không có step `Pre-install apt packages`, `Pre-install SDL3 dev`, `Pre-install emsdk`, `Verify build.sh detection`. Chỉ còn: checkout → cache → build.sh → upload.

**Nguyên tắc 2 — Không sinh file on-the-fly, chỉ copy:**
- `build.sh` thay `generate_desktop_icon()` (dùng `iconutil`/`rsvg-convert`/`magick`) bằng `copy_desktop_icon()`:
  ```bash
  case "$OS_NAME" in
      macos) cp "$BRANDKIT_DIR/logo.icns" "$out_dir/cTetris.icns" ;;
      windows) cp "$BRANDKIT_DIR/logo.ico" "$out_dir/cTetris.ico" ;;
      linux) cp "$BRANDKIT_DIR/logo.svg" "$out_dir/cTetris.svg" ;;
  esac
  ```
- `appTree.md` show `brandkit/` đã có sẵn `logo.icns`, `logo.ico`, `logo.svg`, `favicon.svg`, `template.svg` — pre-created, commit thẳng vào repo.

**Nguyên tắc 3 — Download/git-clone trực tiếp vào `libs/<os>/downloads/`:**
- `appTree.md` cho thấy `app/libs/macos/downloads/` chứa `SDL-3.4.4/` (full source clone) và `sdl3-wasm-3.4.4/` (built artifacts).
- `ensure_emsdk()`, `build_sdl3_from_source()` clone trực tiếp vào `$DOWNLOAD_DIR`, không qua `brew`/`apt`.

## 2. Hiện tượng của lỗi cần fix

- ở trình duyệt có login tài khoản, khi tôi mở trang https://phuctanpham.github.io/ctetris-2/ctetris.html và load ra như web như commit cũ (be8068b) trong khi tôi mở trình duyệt ẩn danh thì nó hiển thị lỗi 404. 
- ở trình duyệt có login tài khoản, Khi tôi thay đổi thành https://phuctanpham.github.io/ctetris-2/ thì nó tự động redirect thành trang https://phuctanpham.github.io/ctetris-2/ctetris.html và load ra như web như commit cũ (be8068b) trong khi tôi mở trình duyệt ẩn danh thì nó không redirect mà hiển hiển web nhưng bị các lỗi như bên dưới:
    - Mất nút Install PWA: Phiên bản hiện tại đã chủ động gỡ bỏ hoàn toàn cơ chế Progressive Web App (PWA) để trình duyệt không hiển thị icon "cài đặt ứng dụng" nữa.
    - Mất hiển thị full-height: Vì không còn được cài đặt như một PWA độc lập (chế độ standalone/fullscreen), ứng dụng giờ đây chạy như một trang web bình thường trên tab. Do đó, thanh địa chỉ (address bar) của trình duyệt xuất hiện và chiếm mất một phần không gian, khiến ứng dụng không thể hiển thị chiều cao toàn màn hình một cách thực thụ. 

## 3. Root cause của 2 lỗi (Install PWA + full-height)

Bạn đã push file `web/shell.html`, `web/manifest.webmanifest`, `web/sw.js` của tôi (đầy đủ PWA). NHƯNG:

**`build.sh` chỉ copy `favicon.svg` vào `build/wasm/<os>/`:**
```bash
if [ -f "$BRAND_LOGO_SVG" ]; then
    cp "$BRAND_LOGO_SVG" "$BUILD_WASM_DIR/favicon.svg"
fi
# KHONG copy manifest.webmanifest, KHONG copy sw.js
```

**`deploy-pages.yml` chỉ stage 4 file vào `public/`:**
```yaml
cp app/build/wasm/ubuntu/cTetris.html  public/index.html
cp app/build/wasm/ubuntu/cTetris.js    public/
cp app/build/wasm/ubuntu/cTetris.wasm  public/
if [ -f app/build/wasm/ubuntu/favicon.svg ]; then
  cp app/build/wasm/ubuntu/favicon.svg public/
fi
# KHONG copy manifest.webmanifest, KHONG copy sw.js
```

**Hệ quả:**
- `shell.html` có `<link rel="manifest" href="manifest.webmanifest">` → browser fetch `https://.../manifest.webmanifest` → **404 Not Found** → không có manifest hợp lệ → **không có nút Install** → không vào được standalone mode → **không full-height**.
- `shell.html` có `navigator.serviceWorker.register('sw.js')` → fetch `sw.js` → **404** → SW không register được → không offline, càng không Install được.

**Tại sao trình duyệt có login lại OK?**
- SW cũ từ commit `be8068b` đã được register trước đó và **chưa unregister**. Browser cache của bạn vẫn giữ SW cũ + cache cũ → vào root → SW cũ redirect/serve `cTetris.html` từ cache.
- Incognito = clean state → không có SW cũ → trực tiếp đến server → file mới không có manifest/sw → bị lỗi.

## 3. Đề xuất fix (theo đúng nguyên tắc của bạn)

`web/manifest.webmanifest` và `web/sw.js` đã pre-created sẵn trong repo (Nguyên tắc 2). Chỉ cần **copy** chúng đi cùng các artifact khác (giống cách `favicon.svg` đang được copy). Không cần install, verify, hay generate gì.

**Thay đổi 1 — `app/build.sh`** (thêm 2 dòng vào `build_wasm()`):
```bash
# Sau dong cp favicon.svg da co san
if [ -f "$APP_DIR/web/manifest.webmanifest" ]; then
    cp "$APP_DIR/web/manifest.webmanifest" "$BUILD_WASM_DIR/manifest.webmanifest"
fi
if [ -f "$APP_DIR/web/sw.js" ]; then
    cp "$APP_DIR/web/sw.js" "$BUILD_WASM_DIR/sw.js"
fi
```

**Thay đổi 2 — `app/build.ps1`** (tương tự trong `Build-Wasm`):
```powershell
$webDir = Join-Path $AppDir 'web'
foreach ($asset in @('manifest.webmanifest', 'sw.js')) {
    $src = Join-Path $webDir $asset
    if (Test-Path $src) {
        Copy-Item -Force $src (Join-Path $BuildWasmDir $asset)
    }
}
```

**Thay đổi 3 — `.github/workflows/deploy-pages.yml`** (mở rộng "Stage Pages artifact"):
```yaml
for asset in manifest.webmanifest sw.js; do
  if [ -f "app/build/wasm/ubuntu/$asset" ]; then
    cp "app/build/wasm/ubuntu/$asset" "public/$asset"
  fi
done
```

Tổng cộng ~10 dòng thêm, không sinh file mới, không gọi tool ngoài, không install gì, không verify gì. Chỉ copy file đã commit sẵn trong `web/`.

## 4. Lưu ý phụ về SW cũ

Sau khi fix và deploy, các browser **đã từng truy cập** site sẽ vẫn dùng SW cũ một thời gian (cho đến khi browser tự update, thường <24h hoặc khi user đóng tab). Để force refresh, có thể test bằng:
- DevTools → Application → Service Workers → "Unregister" + "Update on reload"
- Hoặc dùng incognito để test ngay (vì incognito đã không có SW cũ — chính là cách bạn phát hiện ra lỗi này).

---

**Xác nhận kỳ vọng giúp tôi:**
1. Bạn hiểu **đúng** 3 nguyên tắc của bạn?
2. Bạn xác định **đúng** root cause?
3. Bạn đã phân tích 3 mong muốn thay đổi tối thiểu ở trên của tôi dưới góc nhìn 1 chuyên gia để đề xuất kế hoạch thay đổi tối ưu hơn.
4. Commit hiện tại đã khắc phục rất nhiều lỗi, tuy nhiên sau khi khắc phục nó lại bị hai lỗi trên mà tôi đã khắc phục trước đó.

Sau khi bạn confirm với tôi và đưa ra kế hoạch thay đổi rồi hãy tiến hành xử lý từng files.