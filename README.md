# BrowserByYuinn

Windows için, C++ ve [Microsoft Edge WebView2](https://learn.microsoft.com/microsoft-edge/webview2/) üzerine inşa edilmiş, native bir masaüstü web tarayıcısı.

Electron gibi çatılar yerine doğrudan Win32 + WebView2 (Chromium tabanlı) tercih edildi; amaç daha düşük RAM/CPU kullanımı ve daha hızlı açılış süresidir. Arayüz (adres çubuğu, sekme şeridi, ayarlar, yeni sekme sayfası, paneller vb.) HTML/CSS/JS ile `assets/` klasöründe yazılır ve WebView2 kontrolleri içinde render edilir; tarayıcı mantığının geri kalanı (pencere yönetimi, veri depolama, güvenlik, otomatik doldurma, engelleme) C++ tarafındadır. Tüm ağır dış bağımlılıklardan (JSON kütüphanesi, UI çatısı, Chromium indirme işi) kaçınılmış, mümkün olan her yer kendi basit kodla ya da Windows'un kendi API'leriyle (DPAPI, LogonUserW, WinHTTP, Shell32) çözülmüştür.

## Özellikler

### Sekmeler
- Sekme açma/kapatma, sabitleme (pin), sürükle-bırak ile sıralama, sekme çoğaltma
- Sekme gruplama (renkli, adlandırılabilir, katlanabilir gruplar)
- Sessize alma (mute) ve sesli sekme göstergesi
- Hover önizleme, sekme arama, uykuda sekme (otomatik bellek tasarrufu) ve "enerji tasarrufu modu"
- Son kapatılan sekmeyi geri alma (Ctrl+Shift+T)
- Oturum/çökme kurtarma: program düzgün kapanmadıysa önceki sekmeler geri yüklenebilir
- Gizli (özel) sekme modu

### Yer imleri ve geçmiş
- Yer imi çubuğu, tek seviyeli klasörleme, sürükle-bırak sıralama
- Okuma listesi
- Geçmiş: zaman aralığı/kategori bazlı temizleme, çıkışta otomatik temizleme, site bazlı hariç tutma listesi, tek kayıt silme

### Şifreler ve otomatik doldurma
- Şifre kasası: DPAPI ile şifreli saklama, güçlü rastgele şifre üretme, sızıntı kontrolü (Have I Been Pwned k-anonymity API'si, varsayılan kapalı)
- Otomatik doldurma: sayfadaki şifre alanları algılanıp kayıtlı hesap önerilir
- Adres/kart kasası (kart numarası DPAPI ile şifreli, kopyala-yapıştır için)
- Hassas bilgiler gösterilmeden önce Windows şifresiyle doğrulama (LogonUserW), otomatik kilitlenme süresi ayarlanabilir

### Gizlilik ve güvenlik
- Reklam engelleme ve izleyici (tracker) engelleme (Kapalı/Temel/Dengeli/Sıkı)
- Güvenli gezinme uyarısı (şüpheli adres desenleri için, tamamen yerel)
- HTTPS zorunlu kılma, adres çubuğunda kilit ikonundan site izinleri paneli
- Sertifika hatası görüntüleyici ve geçerli sitelerde sertifika detayları
- Site izinleri kasası (kamera, mikrofon, konum vb. kararların kalıcı yönetimi)
- İlk kurulumda ve her güncellemede zorunlu gizlilik rıza onayı

### İndirmeler
- Duraklat/devam ettir/iptal, klasörde göster, tamamlanma bildirimi
- Riskli dosya uzantısı (exe, msi, bat, ps1 vb.) için onay istemi
- İndirme konumunu her seferinde sorma seçeneği

### Sayfa araçları
- Yazdır/PDF kaydet, yerleşik PDF görüntüleyici
- Sayfa içi arama (Ctrl+F), sayfa kaynağını görüntüleme (Ctrl+U)
- Sesli okuma, okuyucu modu
- Resimde ters görsel arama (sağ tık menüsü)
- Adres çubuğunda hesap makinesi / birim çevirici

### Kişiselleştirme ve erişilebilirlik
- Açık/Koyu/Otomatik/Yüksek Kontrast tema, serbest vurgu rengi seçici, üç hazır tema
- Köşe yuvarlaklığı, yazı boyutu, sekme yoğunluğu ayarları
- Yeni sekme sayfasına özel arka plan fotoğrafı, en çok ziyaret edilen siteler
- Özelleştirilebilir klavye kısayolları
- Siteye özel zorunlu koyu mod (geçici, sayfa bazlı)
- İlk açılışta karşılama (onboarding) ekranı

### Uzantılar ve betikler
- Chrome Web Mağazası adresinden uzantı yükleme (CRX indirip paketten çıkarma) ve paketlenmemiş uzantı yükleme
- Kendi basit userscript sistemi (desen + JavaScript, site bazlı otomatik enjeksiyon)

### Diğer
- Çoklu profil desteği (her profil kendi veri klasöründe tamamen izole)
- Gömülü DevTools (F12)
- Varsayılan tarayıcı yapma istemi
- Günlük bazlı hata/olay kaydı (NET-1001 gibi sabit kodlarla), Ayarlar > Hata Günlüğü panelinden görüntülenebilir
- Program sürümü .exe'ye gömülüdür; gizlilik rıza onayı ve güncelleme kontrolleri buna göre çalışır

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

## Proje yapısı

- `src/` — C++ kaynak kodu (pencere yönetimi, WebView2 entegrasyonu, depolama, güvenlik, kasa modülleri)
- `assets/` — toolbar, ayarlar, yeni sekme, paneller ve diğer tüm arayüz sayfaları (HTML/CSS/JS), `tarayici.local` sahte adresi üzerinden WebView2'ye yüklenir
- `thirdparty/` — WebView2 SDK (repoya dahil değil, elle eklenir)
- `yapilanlar.md` — projenin adım adım geliştirme geçmişi ve her adımda alınan teknik kararların ayrıntılı kaydı

## Lisans

Bu proje [GNU General Public License v3.0](LICENSE) ile lisanslanmıştır.
