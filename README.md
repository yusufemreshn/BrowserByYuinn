# BrowserByYuinn

Windows için, C++ ve [Microsoft Edge WebView2](https://learn.microsoft.com/microsoft-edge/webview2/) üzerine inşa edilmiş, native bir masaüstü web tarayıcısı.

Electron gibi çatılar yerine doğrudan Win32 + WebView2 (Chromium tabanlı) tercih edildi; amaç daha düşük RAM/CPU kullanımı ve daha hızlı açılış süresidir. Arayüz (adres çubuğu, sekme şeridi, ayarlar, yeni sekme sayfası vb.) HTML/CSS/JS ile `assets/` klasöründe yazılır ve WebView2 kontrolleri içinde render edilir; tarayıcı mantığının geri kalanı (pencere yönetimi, veri depolama, güvenlik, otomatik doldurma) C++ tarafındadır.

## Özellikler

- Sekme yönetimi: sabitleme, gruplama, sürükle-bırak sıralama, sessize alma, hover önizleme, son kapatılan sekmeyi geri alma, oturum/çökme kurtarma, gizli sekme
- Yer imleri: klasörleme, yer imi çubuğu, okuma listesi
- Şifre ve otomatik doldurma kasası: DPAPI ile şifreleme, sızıntı/zayıf şifre kontrolü, adres/kart bilgisi kasası, otomatik kilitlenme
- Gizlilik ve güvenlik: reklam/izleyici engelleme (Temel/Dengeli/Sıkı), güvenli gezinme uyarısı, HTTPS zorunlu kılma, sertifika görüntüleyici, site izinleri paneli
- Geçmiş yönetimi: zaman aralığı/kategori bazlı temizleme, çıkışta otomatik temizleme, site bazlı hariç tutma listesi
- İndirme yöneticisi: duraklat/devam/iptal, riskli dosya uyarısı, tamamlanma bildirimi
- Sayfa araçları: yazdır/PDF, sayfa çevirisi, sesli okuma, okuyucu modu, ters görsel arama
- Uzantı desteği: Chrome Web Mağazası'ndan yükleme + kendi küçük eklenti sistemi
- Erişilebilirlik: yüksek kontrast modu, özelleştirilebilir klavye kısayolları, siteye özel zorunlu koyu mod
- Çoklu profil desteği, gömülü DevTools (F12)

## Derleme

Gereksinimler:

- Windows 10/11, Visual Studio 2022 (Desktop development with C++ iş yükü)
- [WebView2 SDK](https://www.nuget.org/packages/Microsoft.Web.WebView2) (NuGet paketi; bu repoya dahil değildir, lisans gereği ayrıca indirilmelidir)
- Bilgisayarda WebView2 Runtime kurulu olmalı (çoğu güncel Windows sisteminde zaten gelir)

Adımlar:

1. `tarayici.sln` dosyasını Visual Studio ile açın.
2. NuGet paket yöneticisinden `Microsoft.Web.WebView2` paketini geri yükleyin (Restore), ya da WebView2 SDK'sını elle indirip `thirdparty/WebView2/include` ve `thirdparty/WebView2/lib/x64` klasörlerine yerleştirin.
3. `x64 / Debug` (veya `Release`) yapılandırmasıyla derleyin.
4. Derlenen `tarayici.exe`, `assets/` klasörünün yanına kopyalanmalıdır (build sonrası adım olarak ayarlanmıştır).

## Lisans

Bu proje [GNU General Public License v3.0](LICENSE) ile lisanslanmıştır.
