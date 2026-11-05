// Bu dosya, tarayıcının ana penceresini yöneten BrowserWindow sınıfının tanımını içerir, yani bu dosyada sınıfın neler yapabildiği listelenir, nasıl yaptığı ise BrowserWindow.cpp dosyasındadır.
// Bu sınıf, Win32 penceresini oluşturmaktan, toolbar arayüzünü gösteren WebView2'yi kurmaktan, birden fazla sekmeyi (her biri kendi WebView2'si olan) yönetmekten, geçmiş ve yer imi kayıtlarını tutmaktan, hata oluştuğunda kullanıcıya kodlu bir hata ekranı göstermekten, dahili bir Ayarlar sekmesi sunmaktan, indirmeleri onaylamaktan, ve uzun süre kullanılmayan sekmeleri uykuya alarak RAM tasarrufu sağlamaktan sorumludur.
// main.cpp dosyası, programın en başında bu sınıftan bir nesne oluşturup Create fonksiyonunu çağırır, ondan sonra tüm pencere ve tarayıcı mantığı bu sınıfın içinde yürür.
// Bu dosya, WebView2 kütüphanesinin kendi başlık dosyasına (WebView2.h), geçmiş/yer imi için Storage.h, hata kaydı için Logger.h, ayarlar için Settings.h, ve şifre yöneticisi için PasswordVault.h dosyalarına ihtiyaç duyar.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <windows.h> // Win32 pencere fonksiyonlarını ve temel Windows türlerini kullanmak için eklenir
#include <wrl.h> // Microsoft'un COM nesnelerini kolay yönetmek için sağladığı ComPtr adlı akıllı işaretçi sınıfını kullanmak için eklenir
#include <WebView2.h> // WebView2'nin kendi arayüzlerini (ICoreWebView2 gibi) tanımlayan başlık dosyası, thirdparty klasöründen alınır
#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla sekmeyi bir liste halinde tutmak için eklenir
#include <memory> // Her sekmenin kendi sabit bellek adresinde kalmasını sağlayan std::unique_ptr'ı kullanmak için eklenir
#include <map> // Bekleyen site izin isteklerini kimlik numaralarına göre tutmak için eklenir

// Bu yapı, açık olan tek bir sekmeyi temsil eder, yani o sekmenin kendi kimlik numarasını, kendi WebView2 denetleyicisini ve WebView2 nesnesini, başlığını, ve performans/dahili durum bilgilerini tutar
struct SekmeBilgisi
{
    int id = 0; // Bu sekmeye ait, programın kendi içinde ürettiği benzersiz kimlik numarası
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> denetleyici; // Bu sekmenin WebView2 denetleyicisi, ekrandaki konumunu ve görünürlüğünü yönetir
    Microsoft::WRL::ComPtr<ICoreWebView2> webView; // Bu sekmenin asıl WebView2 nesnesi, gezinme işlemleri bunun üzerinden yapılır
    Microsoft::WRL::ComPtr<ICoreWebView2_4> webViewGenisletilmis; // Aynı WebView2'nin, SetVirtualHostNameToFolderMapping, TrySuspend ve DownloadStarting gibi daha yeni özelliklere sahip genişletilmiş sürümü
    std::wstring baslik = L"Yeni Sekme"; // Bu sekmenin güncel başlığı, sekme şeridinde gösterilir
    bool yukleniyor = false; // Bu sekmenin şu anda bir sayfa yükleyip yüklemediği
    bool dahiliSayfaMi = false; // Bu sekmenin, programın kendi dahili sayfalarından (örneğin Ayarlar) birini mi gösterdiği, yoksa normal bir internet sitesi mi gösterdiği
    bool askidaMi = false; // Bu sekmenin, RAM tasarrufu için şu anda uykuya alınmış (askıya alınmış) olup olmadığı
    bool gizliMi = false; // Bu sekmenin bir gizli (özel) sekme olup olmadığı, gizli sekmelerde ziyaret edilen sayfalar geçmişe kaydedilmez
    bool sabitliMi = false; // Bu sekmenin sabitlenmiş (pinned) olup olmadığı, sabitlenmiş sekmeler şeritte en solda, daha küçük ve sadece simgeyle gösterilir
    bool sessizeAlindiMi = false; // Kullanıcının bu sekmeyi elle sessize alıp almadığı, WebView2'nin IsMuted özelliğiyle eşlenir
    bool sesCaliyorMu = false; // Bu sekmenin şu anda ses çalıp çalmadığı (video/müzik gibi), sekme şeridinde ses ikonu göstermek için kullanılır
    int grupId = 0; // Bu sekmenin ait olduğu sekme grubunun kimliği, 0 ise hiçbir gruba ait değildir
    std::wstring favicon; // Sitenin kendi favicon'u, "data:image/...;base64,..." biçiminde bir hazır veri adresi olarak tutulur, site henüz favicon vermediyse boştur ve sekme şeridinde programın kendi logosu gösterilir
    std::wstring sertifikaKonu; // Geçerli (hatasız) bir HTTPS bağlantısının sertifika "subject" (konu) bilgisi, CDP Network.responseReceived olayından doldurulur, her yeni gezinmede temizlenir
    std::wstring sertifikaYayinlayan; // Aynı sertifikanın yayınlayan (issuer) bilgisi
    std::wstring sertifikaProtokol; // Bağlantıda kullanılan TLS protokol sürümü (örn. "TLS 1.3")
    std::wstring guvenliGezinmeIstisnaAdresi; // Kullanıcı güvenli gezinme uyarısında "Yine de Git" dediğinde, bu TEK adrese bir dahaki NavigationStarting'de tekrar uyarı gösterilmez
    ULONGLONG sonAktifZaman = 0; // Bu sekmenin en son ne zaman aktif/görünür sekme olduğu, GetTickCount64() değeri olarak tutulur, uzun süre aktif olmayan sekmeler uykuya alınır
    // ÖNEMLİ: denetleyici, webView ve webViewGenisletilmis alanları, bu sekme sekme şeridinde göründükten KISA bir süre sonra dolar, çünkü artık bir sekme, gerçek WebView2 denetleyicisi hazır olmadan ÖNCE şeritte görünebiliyor
    // Bu yüzden bu üç alana erişen HER yer, önce bunların boş olup olmadığını kontrol etmek zorundadır, aksi halde denetleyici henüz hazırlanıyorken yapılan bir erişim programı çökertir
};

// Bu yapı, kullanıcıya henüz sorulmamış, cevap bekleyen tek bir site izin isteğini temsil eder
// WebView2, bir izin isteğini ertelediğimizde (GetDeferral), kullanıcı karar verene kadar bu isteği ve ertelemeyi canlı tutmamızı bekler, bu yapı tam olarak bunun için saklanır
struct BekleyenIzinIstegi
{
    Microsoft::WRL::ComPtr<ICoreWebView2PermissionRequestedEventArgs> argumanlar; // İsteğin kendisi, karar verildiğinde put_State ile sonucu bildirmek için saklanır
    Microsoft::WRL::ComPtr<ICoreWebView2Deferral> erteleme; // Kullanıcı karar verene kadar WebView2'nin bu isteği beklemede tutmasını sağlayan erteleme nesnesi
    int sekmeId = 0; // Bu isteğin geldiği sekmenin kimlik numarası
};

// Bu yapı, bir sitenin sertifikası güvenilmez/hatalı bulunduğunda, kullanıcıya henüz sorulmamış, cevap bekleyen tek bir sertifika hatası bildirimini temsil eder
// WebView2, bir sertifika hatasını ertelediğimizde (GetDeferral), kullanıcı karar verene kadar bu isteği ve ertelemeyi canlı tutmamızı bekler, bu yapı tam olarak bunun için saklanır
struct BekleyenSertifikaHatasi
{
    Microsoft::WRL::ComPtr<ICoreWebView2ServerCertificateErrorDetectedEventArgs> argumanlar; // Hatanın kendisi, karar verildiğinde put_Action ile sonucu bildirmek için saklanır
    Microsoft::WRL::ComPtr<ICoreWebView2Deferral> erteleme; // Kullanıcı karar verene kadar WebView2'nin bu gezinmeyi beklemede tutmasını sağlayan erteleme nesnesi
    int sekmeId = 0; // Bu hatanın geldiği sekmenin kimlik numarası
};

// Bu yapı, kapatılmış bir sekmenin, "Son Kapatılan Sekmeyi Aç" özelliği için yeniden açılabilmesini sağlayacak minimum bilgisini (adresini) tutar
// Sekmenin geçmişi (geri/ileri listesi) veya kaydırma konumu gibi diğer durumları KORUNMAZ, sadece son gösterdiği adrese yeniden gidilir, bu bilinçli bir basitlik kararıdır
struct KapatilanSekmeBilgisi
{
    std::wstring adres; // Kapatılan sekmenin son gösterdiği adres
    bool dahiliMi = false; // Kapatılan sekmenin, programın kendi dahili bir sayfası (örneğin Ayarlar) olup olmadığı, yeniden açılırken aynı ayrıcalıkla açılması için saklanır
};

// Bu yapı, devam eden veya tamamlanmış tek bir indirmeyi temsil eder, İndirmeler panelinde listelenir
struct IndirmeBilgisi
{
    int id = 0; // Bu indirmeye ait, programın kendi içinde ürettiği benzersiz kimlik numarası
    Microsoft::WRL::ComPtr<ICoreWebView2DownloadOperation> islem; // WebView2'nin kendi indirme işlemi nesnesi, ilerleme/durum okumak ve duraklat/devam/iptal için kullanılır
    std::wstring dosyaAdi; // İndirilen dosyanın, tam yoldan ayıklanmış sade adı
    std::wstring hedefYol; // İndirilen dosyanın diskteki tam yolu, "Klasörde Göster" için kullanılır
    bool tamamlanmaBildirimiGosterildi = false; // Bu indirme için tamamlanma bildiriminin zaten gösterilip gösterilmediği, StateChanged birden fazla kez tetiklenirse bildirim tekrarlanmasın diye
};

// Bu sınıf, tarayıcının tek bir ana penceresini, toolbar arayüzünü, ve içindeki tüm sekmeleri temsil eder
class BrowserWindow
{
public:
    // Bu fonksiyon, gerçek Win32 penceresini oluşturur ve WebView2 ortamının hazırlanmasını başlatır, programın en başında bir kere çağrılır
    bool Create(HINSTANCE uygulamaOrnegi);

private:
    // Bu fonksiyon, Windows'tan bu pencere sınıfına ait her mesajı ilk karşılayan statik fonksiyondur, Windows sadece statik fonksiyonları doğrudan çağırabildiği için bu şekilde yazılmıştır
    static LRESULT CALLBACK StatikPencereMesajIsleyici(HWND pencere, UINT mesaj, WPARAM birinciParametre, LPARAM ikinciParametre);

    // Bu fonksiyon, statik mesaj işleyiciden çağrılan, asıl mesaj işleme mantığının bulunduğu, sınıfın kendi verilerine erişebilen fonksiyondur
    LRESULT PencereMesajIsle(HWND pencere, UINT mesaj, WPARAM birinciParametre, LPARAM ikinciParametre);

    // Bu fonksiyon, WebView2 ortamının oluşturulmasını başlatır, ortam hazır olduğunda da toolbar WebView2'sinin ve ilk sekmenin oluşturulmasını tetikler
    void WebView2OrtamiBaslat();

    // Bu fonksiyon, toolbar arayüzünü gösterecek WebView2 denetleyicisini ve WebView2 nesnesini oluşturur
    void ToolbarWebViewOlustur();

    // Bu fonksiyon, hamburger menü panelini gösterecek AYRI, küçük WebView2 denetleyicisini oluşturur, başlangıçta gizlidir
    void MenuWebViewOlustur();

    // Bu fonksiyon, hamburger menü panelini açar: denetleyicinin boyutunu/konumunu hesaplayıp görünür yapar, güncel RAM ve yakınlaştırma bilgisini gönderir
    void MenuyuAc();

    // Bu fonksiyon, hamburger menü panelini kapatır (gizler)
    void MenuyuKapat();

    // Bu fonksiyon, menü panelinin WebView2'sinin gerçek alt penceresini bulup üzerine yuvarlak köşeli bir SetWindowRgn uygular
    // menu.css'teki border-radius sadece ÇİZİMİ yuvarlatır, pencerenin GERÇEK dikdörtgen sınırını değiştirmez; bu yüzden köşelerde eski köşeli pencerenin kendisi (genelde siyah) görünmeye devam eder
    // Bu fonksiyon o gerçek pencereyi de aynı yarıçapla kırparak, köşelerin dışında kalan kısmın hiç çizilmemesini (altındaki sayfanın görünmesini) sağlar
    void MenuPaneliKoseleriniYuvarlaYap(const RECT& menuAlaniIstemciKoordinati);

    // Bu fonksiyon, native taraftan hamburger menü paneline bir metin mesajı gönderir
    void MenuyeMesajGonder(const std::wstring& mesaj);

    // Bu fonksiyon, verilen adreste yeni bir sekme açar, istenirse bu sekmeyi hemen aktif sekme yapar, dahiliMi doğruysa bu sekmeye yerel tarayici.local adresine erişim ve ayrıcalıklı mesajlaşma izni verilir, gizliMi doğruysa bu sekmede gezinme geçmişe kaydedilmez, yeni sekmenin kimlik numarasını hemen döndürür
    // Bu fonksiyon çağrıldığı anda, henüz hiçbir WebView2 denetleyicisi hazır olmadan, boş bir yer tutucu sekme sekme şeridine ANINDA eklenir, gerçek denetleyici arkadan (havuzdan veya sıfırdan) hazırlanıp birkaç saniye içinde bu sekmeye bağlanır
    int YeniSekmeOlustur(const std::wstring& baslangicAdresi, bool aktifYap, bool dahiliMi, bool gizliMi = false);

    // Bu fonksiyon, hazır bir WebView2 denetleyicisi (ister az önce sıfırdan oluşturulmuş, ister önceden hazırlanmış bir yedek olsun) verilince, bu denetleyiciyi YeniSekmeOlustur tarafından önceden şeride eklenmiş yer tutucu sekmeye bağlar, olay dinleyicilerini bağlar ve verilen adrese gezinir
    // zatenYuklenmisMi doğruysa, bu denetleyici bu adrese arka planda zaten önceden gezindirilmiştir, bu yüzden pahalı bir tekrar gezinme yapılmaz, bunun yerine sayfanın ihtiyaç duyduğu veriler (hızlı erişim listesi, tema) doğrudan gönderilir
    void SekmeDenetleyicisiHazirlandi(ICoreWebView2Controller* denetleyici, int yeniId, const std::wstring& baslangicAdresi, bool aktifYap, bool dahiliMi, bool gizliMi, bool zatenYuklenmisMi);

    // Bu fonksiyon, arka planda, görünmez, kullanıma hazır tek bir yedek WebView2 denetleyicisi oluşturur, çünkü yeni bir denetleyici oluşturmak gözle görülür bir gecikmeye sebep olur, bu yüzden bu gecikme kullanıcı yeni bir sekme açmadan ÖNCE, arka planda yaşanmış olur
    void YedekSekmeHazirla();

    // Bu fonksiyon, verilen kimlik numarasına sahip sekmeyi sekme şeridinde seçili yapar, bu sekmenin denetleyicisi zaten hazırsa önceki görünen sekmeyi gizleyip yenisini gösterir ve uykudaysa önce uyandırır
    // Eğer bu sekmenin denetleyicisi henüz hazırlanıyorsa (yeni açılmış bir sekmeyse), ekrandaki görünüm bilerek değiştirilmez, sadece şeritteki seçim güncellenir, gerçek görsel geçiş denetleyici hazır olunca SekmeDenetleyicisiHazirlandi tarafından kendiliğinden yapılır
    void SekmeyeGec(int sekmeId);

    // Bu fonksiyon, verilen kimlik numarasına sahip sekmeyi kapatır, eğer kapatılan sekme aktif sekmeyse başka bir sekmeye geçilir
    void SekmeKapat(int sekmeId);

    // Bu fonksiyon, verilen kimlik numarasına sahip sekmenin bilgilerini listeden bulup işaretçisini döndürür, bulunamazsa boş işaretçi döndürür
    SekmeBilgisi* SekmeBul(int sekmeId);

    // Bu fonksiyon, şu anda aktif olan sekmenin bilgilerini döndürür, hiçbir sekme aktif değilse boş işaretçi döndürür
    SekmeBilgisi* AktifSekmeyiAl();

    // Bu fonksiyon, yeni oluşturulan bir sekmenin WebView2'sine, sayfa geçişi, başlık değişimi, çökme ve indirme gibi olayları dinleyen işleyicileri bağlar
    void SekmeOlaylariniBagla(int sekmeId);

    // Bu fonksiyon, pencere boyutu değiştiğinde çağrılır, toolbar WebView2'sinin üstte sabit bir yükseklikte, aktif sekmenin WebView2'sinin ise kalan tüm alanda doğru şekilde konumlanmasını sağlar
    void BoyutlariGuncelle();

    // Bu fonksiyon, toolbar arayüzünden (JavaScript tarafından postMessage ile) gelen metin mesajını okuyup, mesajın içeriğine göre doğru işlemi yapar
    void ToolbarMesajiIsle(const std::wstring& mesaj);

    // Bu fonksiyon, dahili bir sayfadan (şu an için sadece Ayarlar sekmesinden) gelen mesajı okuyup, ayarlar/geçmiş/yer imi/şifre/günlük ile ilgili doğru işlemi yapar, bu fonksiyon normal internet sitelerinden gelen mesajlar için asla çağrılmaz
    void DahiliSayfaMesajiIsle(int sekmeId, const std::wstring& mesaj);

    // Bu fonksiyon, toolbar arayüzüne (JavaScript tarafına) native taraftan bir metin mesajı gönderir
    void ToolbaraMesajGonder(const std::wstring& mesaj);

    // Bu fonksiyon, verilen sekmeye, native taraftan bir metin mesajı gönderir, Ayarlar sekmesiyle konuşmak için kullanılır
    void SekmeyeMesajGonder(int sekmeId, const std::wstring& mesaj);

    // Bu fonksiyon, adres çubuğuna yazılan metnin bir web adresi mi yoksa bir arama sorgusu mu olduğuna karar verir, arama sorgusuysa kullanıcının seçtiği arama motoruna çevirir, sonra aktif sekmede bu adresi açar
    void AkilliAdreseGit(const std::wstring& girilenMetin);

    // Bu fonksiyon, aktif sekmedeki sayfa değiştiğinde çağrılır, güncel adresi, geri/ileri gidilip gidilemeyeceği, ve yer imi durumunu toolbar arayüzüne gönderir
    void GezinmeDurumunuGuncelleVeGonder();

    // Bu fonksiyon, açık olan tüm sekmelerin kimlik numarası, başlığı, yükleniyor durumu ve uykuda olup olmadığı bilgisini, aktif sekmenin kimliğiyle birlikte toolbar arayüzüne gönderir
    void SekmeListesiniGonder();

    // Bu fonksiyon, fare bir sekmenin üzerinde beklerken çağrılır, verilen sekmenin güncel görünümünün küçük bir anlık görüntüsünü (PNG) yakalayıp, Base64 ile kodlanmış halde toolbar arayüzüne gönderir, bu da sekmenin üzerine gelince küçük bir önizleme olarak gösterilir
    void SekmeOnizlemesiGonder(int sekmeId);

    // Bu fonksiyon, verilen sekmede, verilen hata koduna sahip, kullanıcıya gösterilecek basit bir hata sayfası açar, ve bu hatayı günlük dosyasına kaydeder, başarısız olan adres varsa yeniden deneme bağlantısı olarak gösterilir
    void HataSayfasiGoster(int sekmeId, const std::wstring& kod, const std::wstring& basarisizAdres);

    // Bu fonksiyon, WebView2'nin kendi ağ hatası durumunu (örneğin adres bulunamadı, bağlantı reddedildi), bizim kendi kategori ve numara içeren hata koduna çevirir
    static std::wstring AgHatasiniKoduCevir(COREWEBVIEW2_WEB_ERROR_STATUS hataDurumu);

    // Bu fonksiyon, kullanıcının seçtiği arama motoruna göre, verilen (zaten adres için kodlanmış) arama metnini tam bir arama adresine çevirir
    static std::wstring AramaAdresiOlustur(const std::wstring& aramaMotoru, const std::wstring& kodlanmisSorgu);

    // Bu fonksiyon, Ayarlar sekmesine, güncel ayarları, geçmişi, yer imlerini, şifreleri veya hata günlüğünü JSON olarak gönderir, komutun ne istediğine göre doğru veriyi hazırlar
    void AyarlarIcinVeriGonder(int sekmeId, const std::wstring& komut);

    // Bu fonksiyon, Geçmiş panelindeki veri temizleme işlevi tarafından çağrılır, verilen zaman aralığı ve kategorilere göre hem WebView2'nin kendi verilerini (çerez, önbellek, indirme geçmişi, otomatik doldurma, site izinleri) hem de bizim kendi tuttuğumuz geçmiş ve şifre kayıtlarını temizler
    // zamanKodu: "1s" (son 1 saat), "24s" (son 24 saat), "7g" (son 7 gün), "4h" (son 4 hafta) veya "tum" (tüm zamanlar) değerlerinden biridir
    // kategorilerMetni: "gecmis,cerez,onbellek,indirme,sifre,doldurma,izinler" gibi virgülle ayrılmış kategori adları içerir
    void VeriTemizle(int sekmeId, const std::wstring& zamanKodu, const std::wstring& kategorilerMetni);

    // Bu fonksiyon, dahili bir Ayarlar sekmesi açar, eğer zaten açıksa var olan sekmeye geçer
    // kategori boş değilse, Ayarlar sayfasının o kategorisi (örneğin "gecmis" veya "sifreler") doğrudan açılır, hamburger menüsündeki kısayol bağlantıları bunu kullanır
    void AyarlarSekmesiniAc(const std::wstring& kategori = L"");

    // Bu fonksiyon, güncel tema ayarlarını (vurgu rengi, sekme yoğunluğu, yazı boyutu) toolbar arayüzüne gönderir, böylece arayüz anında güncellenir
    void TemayiToolbaraGonder();

    // Bu fonksiyon, kayıtlı yer imlerini okuyup toolbar arayüzündeki yer imi çubuğuna gönderir, bir yer imi eklendiğinde/kaldırıldığında/taşındığında/sıralaması değiştiğinde çağrılır
    void BookmarkBariGuncelleVeGonder();

    // Bu fonksiyon, bir dosya indirmesi başladığında çağrılır, çalıştırılabilir dosya uzantıları için kullanıcıya bir onay sorusu sorar, onaylanmazsa indirmeyi iptal eder, aksi halde indirmeyi listeye ekleyip takip etmeye başlar
    void IndirmeBasladi(ICoreWebView2DownloadStartingEventArgs* argumanlar);

    // Bu fonksiyon, güncel indirme listesini (ilerleme, durum dahil) Ayarlar sekmesindeki İndirmeler paneline gönderir
    void IndirmeListesiniGonder();

    // Bu fonksiyon, verilen kimlik numaralı indirmeyi duraklatır, devam ettirir veya iptal eder, eylem "pause", "resume" veya "cancel" olabilir
    void IndirmeEylemiUygula(int indirmeId, const std::wstring& eylem);

    // Bu fonksiyon, verilen kimlik numaralı indirmenin dosyasını, Windows Gezgini'nde seçili olarak gösterir
    void IndirmeyiKlasordeGoster(int indirmeId);

    // Bu fonksiyon, programın bildirim alanı (sistem tepsisi) simgesini oluşturur, program açılışında bir kere çağrılır, simge hiçbir zaman görünür bir simge olarak görev çubuğunda durmaz, sadece bildirim göstermek için kullanılır
    void BildirimSimgesiOlustur();

    // Bu fonksiyon, bildirim alanında, verilen dosya adıyla bir "indirme tamamlandı" balonu gösterir
    void IndirmeTamamlandiBildirimiGoster(const std::wstring& dosyaAdi);

    // Bu fonksiyon, uzun süredir aktif olmayan sekmeleri bulup uykuya alır, pencerenin arka planında belirli aralıklarla periyodik olarak çalışır
    void UykudaSekmeleriKontrolEt();

    // Bu fonksiyon, programın ve ona bağlı tüm WebView2 süreçlerinin topladığı toplam RAM kullanımını megabayt cinsinden hesaplar
    static DWORD ToplamRamKullanimiMB();

    // Bu fonksiyon, basılan bir klavye kısayolunun (her zaman Ctrl ile başlar) tarayıcıya özel bir komuta karşılık gelip gelmediğini kontrol eder, karşılık geliyorsa ilgili işlemi yapar ve doğru değerini döndürür, böylece bu tuş sayfaya hiç gönderilmez
    bool KisayolTusuIsle(UINT32 tusKodu);

    // Bu fonksiyon, verilen sekmenin yakınlaştırma oranını, verilen miktar kadar artırır veya azaltır, oran belirli bir alt ve üst sınırın dışına çıkamaz
    static void ZoomDegistir(SekmeBilgisi* sekme, double miktar);

    // Bu fonksiyon, aktif sekmenin içine, basit bir sayfa içi arama çubuğu enjekte eder, çubuk zaten açıksa kapatır
    void SayfaIciAramaCubuguAcKapat();

    // Bu fonksiyon, aktif sekmenin görünür metnini, Web Speech API (window.speechSynthesis) ile sesli okumaya başlar, zaten okunuyorsa durdurur
    void SesliOkumayiBaslatDurdur();

    // Bu fonksiyon, aktif sekmedeki sayfanın ana metin içeriğini basit bir sezgisel yöntemle bulup, reklam/menü gibi dikkat dağıtan öğeler olmadan, sade bir okuma görünümünde gösterir
    void OkuyucuModunuAc();

    // Bu fonksiyon, aktif sekmedeki sayfayı, sayfanın kendi koyu temasından bağımsız olarak zorla koyu renklere çevirir (renkleri tersine çevirip tekrar görsellerde düzelten klasik yöntemle), tekrar çağrılırsa geri alır
    void ZorunluKoyuModuAcKapat();

    // Bu fonksiyon, yüklü tüm paketlenmemiş tarayıcı uzantılarının listesini (ad, kimlik, etkin olup olmadığı) verilen sekmeye JSON olarak gönderir
    void EklentileriniGonder(int sekmeId);

    // Bu fonksiyon, kullanıcının bir klasör seçmesini ister, seçilen klasörü paketlenmemiş bir uzantı olarak yüklemeye çalışır, sonucu (başarılı/başarısız) günceller
    void EklentiKlasoruSecVeYukle(int sekmeId);

    // Bu fonksiyon, verilen kimlikli uzantıyı kaldırır veya etkinliğini değiştirir, eylem "remove", "enable" veya "disable" olabilir
    void EklentiyiDegistir(int sekmeId, const std::wstring& eklentiId, const std::wstring& eylem);

    // Bu fonksiyon, verilen bir Chrome Web Mağazası sayfası adresinden uzantı kimliğini çıkarır, CRX paketini arka planda indirip paketini açar ve yükler, sonucu asenkron olarak MagazaEklentiSonucuMesaji ile bildirir
    // Bu, Google'ın resmi desteklediği bir yol DEĞİLDİR, WebView2 tabanlı uygulamalar Chrome Web Mağazası'nın gerçek Chrome'a özel yükleme akışını kullanamaz
    void EklentiMagazaAdresindenYukle(int sekmeId, const std::wstring& magazaUrl);

    // Bu fonksiyon, mevcut tüm profillerin (AppData\tarayici\profiller altındaki alt klasörlerin) adlarını ve şu an aktif olan profilin adını, verilen sekmeye JSON olarak gönderir
    void ProfilListesiniGonder(int sekmeId);

    // Bu fonksiyon, verilen adla yeni bir süreç olarak programı yeniden başlatır (--profil=<ad> argümanıyla) ve ardından BU süreci (bu pencereyi) kapatır
    // WebView2 ortamı süreç başına tek olduğu için, canlı (yeniden başlatmasız) profil değişimi bu mimaride desteklenmez, bu basit ve güvenilir yöntem tercih edilmiştir
    void ProfileGecisYapVeYenidenBaslat(const std::wstring& profilAdi);

    // Bu fonksiyon, Windows şifresiyle doğrulandıktan sonra verilen profilin klasörünü (ve içindeki tüm WebView2 verisini) kalıcı olarak siler, aktif profil silinemez
    void ProfiliSil(int sekmeId, const std::wstring& profilAdi);

    // Bu fonksiyon, "resimler" klasöründeki tüm resimlerin dosya adlarını, Ayarlar sayfasındaki galerinin çizebilmesi için JSON olarak gönderir
    void ArkaPlanGalerisiniGonder(int sekmeId);

    // Bu fonksiyon, kullanıcı Ayarlar sayfasındaki galeriden bir resme tıkladığında, o resmi (dosya adını "resimler" klasörüyle birleştirerek) yeni sekme arka planı olarak kaydeder
    void KlasordenArkaPlanSec(int sekmeId, const std::wstring& dosyaAdi);

    // Bu fonksiyon, dahili bir klavye kısayolları yardım sekmesi açar, eğer zaten açıksa var olan sekmeye geçer
    void KisayollarSekmesiniAc();

    // Bu fonksiyon, verilen kimlik numaralı yeni bir özel (gizli) sekme açar, bu sekmede gezinme geçmişe kaydedilmez
    void GizliSekmeAc();

    // Bu fonksiyon, gömülü DevTools panelini gösterecek AYRI denetleyiciyi oluşturur, program açılışında bir kere çağrılır, panel bu aşamada henüz görünmezdir
    void DevToolsWebViewOlustur();

    // Bu fonksiyon, site izin bildirim panelini gösterecek AYRI denetleyiciyi oluşturur, program açılışında bir kere çağrılır, panel bu aşamada henüz görünmezdir
    void IzinWebViewOlustur();

    // Bu fonksiyon, adres çubuğundaki kilit ikonuna tıklanınca açılan site bilgisi/izinleri panelini gösterecek AYRI denetleyiciyi oluşturur, program açılışında bir kere çağrılır, panel bu aşamada henüz görünmezdir
    void SiteBilgisiWebViewOlustur();

    // Bu fonksiyon, site bilgisi panelini aktif sekmenin bilgileriyle açar
    void SiteBilgisiPaneliniAc();

    // Bu fonksiyon, site bilgisi panelini kapatır (gizler)
    void SiteBilgisiPaneliniKapat();

    // Bu fonksiyon, panelde gösterilen sitenin (m_sitePaneliKaynagi) güvenlik durumunu ve bu siteye ait kayıtlı izin kararlarını JSON olarak panele gönderir
    void SiteBilgisiPaneliVeriGonder();

    // Bu fonksiyon, sertifika hatası bildirim panelini gösterecek AYRI denetleyiciyi oluşturur, program açılışında bir kere çağrılır, panel bu aşamada henüz görünmezdir
    void SertifikaWebViewOlustur();

    // Bu fonksiyon, gösterilecek bekleyen bir sertifika hatası varsa (bir önceki hata sonuçlandırıldıktan sonra sıradaki dahil) sertifika panelini o hatanın bilgileriyle gösterir, hiç bekleyen hata yoksa paneli gizler
    void SertifikaPanelindekiHatayiGuncelle();

    // Bu fonksiyon, sertifika hatası panelindeki "Geri Dön" veya "Yine de Git" butonuna tıklandığında çağrılır, verilen kimlikli hatayı sonuçlandırır
    void SertifikaHatasiYanitla(int istekId, bool devamEt);

    // Bu fonksiyon, WebView2'nin ham sertifika hatası durumunu, kullanıcıya gösterilecek sade bir Türkçe açıklamaya çevirir
    static std::wstring SertifikaHatasiAciklamasi(COREWEBVIEW2_WEB_ERROR_STATUS hataDurumu);

    // Bu fonksiyon, otomatik doldurma panelini gösterecek AYRI denetleyiciyi oluşturur, program açılışında bir kere çağrılır, panel bu aşamada henüz görünmezdir
    void DoldurmaWebViewOlustur();

    // Bu fonksiyon, bir sayfadaki şifre alanına tıklanınca, o sayfadan gelen (ayrıcalıksız) mesajı işler, biçimi "autofill_check|<json>" şeklindedir, json alanın ekran koordinatlarını içerir
    // gercekKaynakAdresi, mesajı gönderen sayfanın GERÇEK adresidir (WebView2'nin kendi get_Source bilgisinden alınır), sayfanın postMessage içine koyduğu bir veri DEĞİLDİR, böylece bir sayfa başka bir sitenin host adını taklit edip kayıtlı şifreleri sorgulayamaz
    void OtomatikDoldurmaIsteginiIsle(int sekmeId, const std::wstring& gercekKaynakAdresi, const std::wstring& mesaj);

    // Bu fonksiyon, otomatik doldurma panelinde bir kullanıcı adı seçildiğinde çağrılır, Windows kimlik doğrulamasından sonra, ilgili şifreyi çözüp isteği yapan sekmeye gönderir
    void OtomatikDoldurmaSecimiYap(const std::wstring& kullaniciAdi);

    // Bu fonksiyon, otomatik doldurma panelini kapatır (gizler)
    void DoldurmaPaneliniKapat();

    // Bu fonksiyon, kullanıcının seçtiği özel arka plan resmini diskten okur, Base64 ile kodlayıp bir "data:" adresi olarak verilen sekmeye gönderir, hiç resim seçilmemişse boş bir adres gönderir
    void YeniSekmeArkaPlaniniGonder(int sekmeId);

    // Bu fonksiyon, açık olan tüm hızlı erişim (yeni sekme) sekmelerine, güncel özel arka plan resmini yeniden gönderir, resim Ayarlar'dan değiştirildiğinde/kaldırıldığında çağrılır
    void YeniSekmeArkaPlaniniTumSekmelereGonder();

    // Bu fonksiyon, kullanıcının elle tek bir resim seçmediği durumda, "resimler" klasöründe bulduğu resim dosyalarından rastgele birinin tam yolunu döndürür, klasör yoksa veya içinde hiç resim yoksa boş metin döndürür
    std::wstring RastgeleKlasorResmiSec();

    // Bu fonksiyon, adres çubuğu öneri panelini gösterecek AYRI denetleyiciyi oluşturur, program açılışında bir kere çağrılır, panel bu aşamada henüz görünmezdir
    void OneriWebViewOlustur();

    // Bu fonksiyon, adres çubuğuna yazılırken gelen "get_addresssuggestions" komutunu işler, geçmiş ve yer imlerinden eşleşenleri bulup paneli gösterir
    void AdresOnerileriniGosterVeGonder(const std::wstring& mesaj);

    // Bu fonksiyon, adres çubuğu öneri panelini kapatır (gizler)
    void OneriPaneliniKapat();

    // Bu fonksiyon, adres çubuğunda yukarı/aşağı ok tuşuna basıldığında öneri panelindeki klavye seçimini bir satır ileri/geri taşır, panel kapalıysa hiçbir şey yapmaz
    void AdresOnerisiGezin(const std::wstring& yon);

    // Bu fonksiyon, Enter'a basıldığında öneri panelinde klavyeyle seçili bir satır varsa ona gider ve true döner, hiçbir satır seçili değilse hiçbir şey yapmadan false döner
    bool AdresOnerisiSeciliyiOnayla();

    // Bu fonksiyon, verilen kimlik numaralı sekmenin sabitlenmiş (pinned) durumunu tersine çevirir, sabitlenmiş sekmeler şeritte en solda ve daha küçük gösterilir
    void SekmeyiSabitleKaldir(int sekmeId);

    // Bu fonksiyon, verilen kimlik numaralı sekmenin güncel adresinde, aynı dahili/gizli durumuyla yeni bir kopya sekme açar, yeni sekme hemen aktif yapılır
    void SekmeyiCogalt(int sekmeId);

    // Bu fonksiyon, verilen kimlik numaralı sekmenin sessize alınmış durumunu tersine çevirir (WebView2'nin IsMuted özelliği üzerinden)
    void SekmeSesiniDegistir(int sekmeId);

    // Bu fonksiyon, verilen sekmeyi içeren yeni bir grup oluşturur, grubun rengi sabit bir paletten otomatik atanır
    void SekmeIcinYeniGrupOlustur(int sekmeId);

    // Bu fonksiyon, verilen sekmeyi, verilen kimlikli mevcut bir gruba ekler
    void SekmeyiGrubaEkle(int sekmeId, int grupId);

    // Bu fonksiyon, verilen sekmeyi bulunduğu gruptan çıkarır, grup bu çıkarmadan sonra boş kalırsa grup tamamen silinir
    void SekmeyiGruptanCikar(int sekmeId);

    // Bu fonksiyon, verilen kimlikli grubun adını değiştirir
    void SekmeGrubunuAdlandir(int grupId, const std::wstring& yeniIsim);

    // Bu fonksiyon, verilen kimlikli gruba ait tüm sekmeleri kapatır
    void SekmeGrubunuKapat(int grupId);

    // Bu fonksiyon, artık hiçbir sekmesi kalmayan grupları listeden temizler
    void BosGruplariTemizle();

    // Bu fonksiyon, reklam/izleyici engelleme ayarlarını diskten okuyup m_reklamEngellemeAcikMi ve m_izleyiciEngellemeSeviyesi önbelleğine yazar, program açılışında ve bu ayarlardan biri değiştiğinde çağrılır
    void EngellemeAyarlariniYenile();

    // Bu fonksiyon, aktif sekme için WebView2'nin kendi yazdırma/PDF kaydetme önizleme penceresini açar, kullanıcı burada fiziksel bir yazıcı veya "PDF olarak kaydet" seçeneği arasında seçim yapabilir
    void SayfayiYazdir();

    // Bu fonksiyon, en son kapatılan sekmeyi (m_kapatilanSekmeler yığınının en üstündekini) yeniden açar, yığın boşsa hiçbir şey yapmaz
    void SonKapatilanSekmeyiAc();

    // Bu fonksiyon, kullanıcının sekme şeridinde sürükleyip bıraktığı yeni sıralamayı uygular, verilen liste m_sekmeler listesindeki TÜM kimliklerin bir permütasyonu olmalıdır, değilse hiçbir şey yapılmaz
    void SekmeleriYenidenSirala(const std::vector<int>& yeniSiraDizisi);

    // Bu fonksiyon, F12 tuşuna basıldığında çağrılır, DevTools paneli kapalıysa açar (ve aktif sekmenin DevTools hedefine bağlanır), açıksa kapatır
    void DevToolsAcKapat();

    // Bu fonksiyon, gösterilecek bekleyen bir izin isteği varsa (bir önceki istek cevaplandıktan sonra sıradaki istek dahil) izin panelini o isteğin bilgileriyle gösterir, hiç bekleyen istek yoksa paneli gizler
    void IzinBildirimPanelindekiIstegiGuncelle();

    // Bu fonksiyon, izin panelindeki "İzin Ver" veya "Engelle" butonuna tıklandığında çağrılır, verilen kimlikli isteği sonuçlandırır, "hatirla" doğruysa kararı kalıcı olarak kaydeder
    void IzinIstegiYanitla(int istekId, bool izinVerildi, bool hatirla);

    HWND m_pencere = nullptr; // Ana Win32 penceresinin tanıtıcısı (handle), pencere oluşturulana kadar boş kabul edilir

    Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewOrtami; // WebView2'nin çalışma ortamı, tüm WebView2 örnekleri bu ortamı paylaşır, böylece her sekme kendi ayrı tarayıcı sürecini başlatmaz

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_toolbarDenetleyici; // Toolbar arayüzünü ekranda gösteren ve konumlandıran denetleyici nesnesi
    Microsoft::WRL::ComPtr<ICoreWebView2> m_toolbarWebView; // Toolbar arayüzünün kendisi, HTML/CSS/JS içeriğiyle gezinme ve mesajlaşma işlemleri bu nesne üzerinden yapılır

    // Hamburger menüsü, toolbar'ın İÇİNDE değil, kendi AYRI küçük WebView2 denetleyicisinde gösterilir
    // Bunun sebebi: toolbar'ı menüyü sığdırmak için büyütüp içeriğin önüne almak hem içeriği aşağı kaydırıyordu hem de (WebView2'nin varsayılan opak arka planı yüzünden) altındaki sayfayı tamamen örtüyordu
    // Ayrı, SADECE menü panelinin gerçek boyutu kadar bir denetleyici kullanmak, bu iki sorunu da kökten çözer: toolbar hiç büyümez, içerik hiç kaymaz, ve menünün kendi küçük dikdörtgeni zaten opak olması GEREKEN bir açılır pencere olduğu için örtme sorunu da yaşanmaz
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_menuDenetleyici; // Hamburger menü panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_menuWebView; // Menü panelinin kendi WebView2 nesnesi

    // Geliştirici araçları (DevTools) paneli de, menü paneliyle aynı sebepten (içeriği kaydırmamak, örtmemek) kendi AYRI denetleyicisinde gösterilir
    // Ama menüden farklı olarak bu panel GEÇİCİ bir açılır pencere değil, KALICI bir yan panel olduğu için, açıkken aktif sekmenin alanını gerçekten daraltır (BoyutlariGuncelle içinde), üzerine binmez
    // İçeriği, WebView2'nin kendi uzaktan hata ayıklama (remote debugging) protokolüyle servis edilen gerçek Chrome DevTools arayüzüdür, bu sayede ayrı bir pencere yerine gerçekten bu pencerenin bir parçası olarak çalışır
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_devToolsDenetleyici; // DevTools panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_devToolsWebView; // DevTools panelinin kendi WebView2 nesnesi, Chrome DevTools arayüzünü (bir web sayfası olarak) gösterir

    // Site izin bildirimi de, menü ve DevTools panelleriyle aynı sebepten (içeriği kaydırmamak, örtmemek) kendi AYRI, küçük denetleyicisinde gösterilir
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_izinDenetleyici; // İzin bildirim panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_izinWebView; // İzin bildirim panelinin kendi WebView2 nesnesi
    std::map<int, BekleyenIzinIstegi> m_bekleyenIzinIstekleri; // Henüz kullanıcıya sorulmamış veya sorulmuş ama cevap bekleyen tüm izin istekleri, kimlik numarasına göre tutulur
    int m_sonrakiIzinIstegiId = 1; // Bir sonraki izin isteğine verilecek kimlik numarası, her yeni istekte bir artırılır

    // Adres çubuğundaki kilit ikonuna tıklanınca açılan site bilgisi/izinleri paneli, aynı sebepten kendi AYRI, küçük denetleyicisinde gösterilir
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_sitePaneliDenetleyici; // Site bilgisi panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_sitePaneliWebView; // Site bilgisi panelinin kendi WebView2 nesnesi
    bool m_sitePaneliAcikMi = false; // Site bilgisi panelinin şu anda açık olup olmadığı
    std::wstring m_sitePaneliKaynagi; // Panelde şu anda gösterilen sitenin kaynağı (origin), "izin kaldır" komutunun hangi siteye uygulanacağını native tarafın kendisi bildiği için, bu bilgi panelden geri istenip güvenilmez şekilde tekrar alınmaz
    ULONGLONG m_sitePaneliSonKapanmaZamani = 0; // Menüdeki aynı LostFocus/tekrar-açma çakışmasını önlemek için, hamburger menüsündeki aynı mantık burada da kullanılır

    // Sertifika hatası bildirimi de, aynı sebepten kendi AYRI, küçük denetleyicisinde gösterilir
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_sertifikaDenetleyici; // Sertifika hatası panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_sertifikaWebView; // Sertifika hatası panelinin kendi WebView2 nesnesi
    std::map<int, BekleyenSertifikaHatasi> m_bekleyenSertifikaHatalari; // Henüz kullanıcıya sorulmamış veya sorulmuş ama cevap bekleyen tüm sertifika hataları, kimlik numarasına göre tutulur
    int m_sonrakiSertifikaHatasiId = 1; // Bir sonraki sertifika hatasına verilecek kimlik numarası, her yeni hatada bir artırılır

    // Bir sayfadaki şifre alanına tıklanınca beliren, kayıtlı hesap önerilerini gösteren otomatik doldurma paneli, aynı sebepten kendi AYRI, küçük denetleyicisinde gösterilir
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_doldurmaDenetleyici; // Otomatik doldurma panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_doldurmaWebView; // Otomatik doldurma panelinin kendi WebView2 nesnesi
    int m_doldurmaSekmeId = -1; // Panelin şu anda hizmet verdiği, isteği yapan sekmenin kimlik numarası
    std::wstring m_doldurmaHost; // Panelin şu anda gösterdiği önerilerin ait olduğu site (host), "doldur" seçimi bu host'a göre doğrulanır
    static const int DoldurmaPaneliGenisligi = 260; // Otomatik doldurma panelinin sabit genişliği
    static const int DoldurmaPaneliYuksekligiSatirBasina = 34; // Her öneri satırının kapladığı yaklaşık yükseklik, panel boyu öneri sayısına göre hesaplanır

    // Adres çubuğu öneri paneli de, aynı sebepten kendi AYRI, küçük denetleyicisinde gösterilir, konumu her açılışta adres çubuğunun o anki ekran konumuna göre hesaplanır
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_oneriDenetleyici; // Öneri panelini gösteren ayrı denetleyici
    Microsoft::WRL::ComPtr<ICoreWebView2> m_oneriWebView; // Öneri panelinin kendi WebView2 nesnesi
    bool m_oneriPaneliAcikMi = false; // Öneri panelinin şu anda açık olup olmadığı
    static const int OneriPaneliSatirYuksekligi = 42; // Her öneri satırının kapladığı GERÇEK yükseklik (CSS'teki iç boşluk + iki satırlık metin + alt çizgi ile birebir), bundan düşük bir değer panelin alt satırları kırpılmış/eksik görünmesine yol açar
    int m_oneriSecilenIndeks = -1; // Panelde klavye ile o an seçili olan satırın sırası (0'dan başlar), hiçbir satır seçili değilse (orijinal yazılan metin geçerliyse) -1
    std::vector<std::wstring> m_oneriSeciliAdresler; // Panelde o an gösterilen (hesap sonucu satırı hariç) her satırın adresi, klavyeyle onaylama bu listeden adres bulur

    std::vector<IndirmeBilgisi> m_indirmeler; // Bu oturumda başlayan tüm indirmelerin listesi, en yeni en sonda
    int m_sonrakiIndirmeId = 1; // Bir sonraki indirmeye verilecek kimlik numarası, her yeni indirmede bir artırılır
    static const UINT BildirimSimgesiMesaji = WM_APP + 2; // Bildirim alanı simgesine yapılan sağ tık gibi etkileşimlerin bu pencereye geri bildirildiği özel mesaj, şu an sadece simgenin var olması için gereklidir, hiçbir menü açmaz

    // Şu anda açık olan tüm sekmelerin listesi, her sekme std::unique_ptr içinde tutulur, böylece listeye yeni sekme eklenirken liste yeniden boyutlansa bile, var olan sekmelerin bellek adresi hiç değişmez, bu da bir sekmenin WebView2 olayları için beklemede olan asenkron geri çağırmaların güvenle çalışmasını sağlar
    std::vector<std::unique_ptr<SekmeBilgisi>> m_sekmeler;

    // Bu yapı, bir sekme grubunu temsil eder: adı ve rengiyle birlikte
    struct SekmeGrubu
    {
        int id = 0; // Grubun benzersiz kimliği
        std::wstring isim = L"Grup"; // Grubun görünen adı, kullanıcı tarafından yeniden adlandırılabilir
        std::wstring renk; // Grubun rengi, onaltılık kod olarak, oluşturulurken sabit bir paletten otomatik atanır
    };
    std::vector<SekmeGrubu> m_sekmeGruplari; // Şu anda var olan tüm sekme gruplarının listesi, bir sekme grubu, en az bir sekmesi kaldığı sürece bu listede durur
    int m_sonrakiGrupId = 1; // Bir sonraki oluşturulacak gruba verilecek kimlik numarası, her yeni grupta bir artırılır

    // Henüz hiçbir sekmeye bağlanmamış, arka planda önceden hazırlanmış, görünmez bir WebView2 denetleyicisi, bir havuzdaki tek bir yedeği temsil eder
    struct YedekSekme
    {
        Microsoft::WRL::ComPtr<ICoreWebView2Controller> denetleyici; // Yedeğin kendi WebView2 denetleyicisi
        ULONGLONG hazirlananZaman = 0; // Bu yedeğin hazır hale geldiği an, uzun süre kullanılmazsa kapatılabilmesi için
    };
    std::vector<YedekSekme> m_yedekler; // Art arda hızlıca açılan birkaç sekmenin de anında açılabilmesi için, tek bir yedek yerine küçük bir havuz tutulur

    std::vector<std::wstring> m_gosterilmemisArkaPlanlar; // "resimler" klasöründeki resimlerden, program bu açılışında henüz hiçbir sekmede gösterilmemiş olanların listesi, RastgeleKlasorResmiSec tarafından kullanılır, bu liste tükenince resimler klasörü yeniden okunup baştan doldurulur (bu sayede hepsi gösterilmeden hiçbiri ikinci kez gösterilmez), sadece bellekte tutulur, program kapanınca sıfırlanır
    static const size_t YedekHavuzuHedefBoyutu = 3; // Aynı anda hazır tutulacak azami yedek sayısı, art arda hızlı sekme açma ile boşta duran RAM arasındaki dengeyi belirler

    // Bu bayrak, pencere kapanmaya başladığında (WM_DESTROY) false yapılır, CapturePreview gibi asenkron WebView2 çağrılarının tamamlanma geri çağırmaları çalışmaya başlamadan önce bu bayrağı kontrol eder
    // Paylaşımlı işaretçi (shared_ptr) olarak tutulması, geri çağırmanın kendi kopyasını yakalayabilmesini sağlar, böylece BrowserWindow nesnesi tamamen yok edilmiş olsa bile bayrağın kendisi hâlâ okunabilir kalır, "this" işaretçisine erişmeden önce güvenle kontrol edilebilir
    std::shared_ptr<bool> m_pencereGecerliMi = std::make_shared<bool>(true);

    // Reklam/izleyici engelleme ayarları, her ağ isteğinde settings.json'u diskten tekrar okumamak için burada önbelleğe alınır, ayar her değiştiğinde EngellemeAyarlariniYenile ile güncellenir
    bool m_reklamEngellemeAcikMi = false;
    std::wstring m_izleyiciEngellemeSeviyesi = L"kapali";

    // Bu bayrak, WM_CLOSE'da yapılan çıkış-zamanı veri temizliğinin sadece bir kez çalışmasını sağlar
    bool m_cikisTemizligiYapildi = false;
    static const size_t YedekHavuzuHicUyumayanSayisi = 1; // Havuzdaki yedeklerden bu kadarı, uzun süre yeni sekme açılmasa bile asla boşta zaman aşımıyla kapatılmaz, böylece kullanıcı uzun bir aradan sonra bile en az bir sekmeyi anında açabilir
    std::wstring m_yedekAdresi; // Yedeklerin arka planda önceden gezindirildiği ortak adres, bir sonraki yeni sekme isteğinin adresiyle aynıysa, pahalı bir tekrar gezinmeden kaçınılabilir
    size_t m_yedekHazirlananSayisi = 0; // Şu anda arka planda, AYNI ANDA kaçtane yedek denetleyicisinin oluşturulmakta olduğunu takip eder, havuzun hedef boyutuna tek tek sırayla değil, hepsi birden paralel olarak dolmasını sağlamak için kullanılır

    // Bu fonksiyon, arka planda uzun süredir kullanılmadan bekleyen yedekleri kapatır ve bellekte tuttukları süreçleri serbest bırakır, böylece kullanıcı uzun süre yeni sekme açmazsa bu RAM boşa harcanmamış olur
    void YedekSekmeyiGerekirseKapat();

    int m_aktifSekmeId = -1; // Şu anda EKRANDA GÖRÜNEN ve denetleyicisi hazır olan sekmenin kimlik numarası, hiçbir sekme yoksa eksi bir değerindedir, bu her zaman denetleyicisi dolu bir sekmeye işaret eder
    int m_secimSekmeId = -1; // Kullanıcının sekme şeridinde SEÇTİĞİ sekmenin kimlik numarası, henüz denetleyicisi hazırlanıyor olsa bile şeritte seçili görünmesi gereken sekme budur, denetleyici hazır olunca m_aktifSekmeId de bu değere eşitlenir
    int m_sonrakiSekmeId = 1; // Bir sonraki oluşturulacak sekmeye verilecek kimlik numarası, her yeni sekmede bir artırılır
    int m_ayarlarSekmeId = -1; // Eğer bir Ayarlar sekmesi açıksa, onun kimlik numarası, hiç açık değilse eksi bir değerindedir
    int m_kisayollarSekmeId = -1; // Eğer bir klavye kısayolları sekmesi açıksa, onun kimlik numarası, hiç açık değilse eksi bir değerindedir

    // Hamburger menü panelinin (ayrı denetleyicisinin) sabit genişliği ve azami yüksekliği, içindeki tüm öğelerin rahatça sığması için yeterli bir pay bırakılır
    static const int MenuPaneliGenisligi = 300;
    static const int MenuPaneliYuksekligi = 720; // Varsayılan yükseklik %50 artırıldı (480 -> 720), menüdeki tüm öğeler kaydırma gerekmeden sığsın diye
    bool m_hamburgerMenusuAcikMi = false; // Hamburger menüsünün şu anda açık olup olmadığı
    // Menü paneli odak kaybıyla (LostFocus) kapandığında bu an kaydedilir
    // Hamburger butonuna tıklamak da önce panelin odağını kaçırıp LostFocus'u tetiklediği için (fare tıklaması odağı mousedown anında değiştirir, "click" olayı daha SONRA gelir),
    // bu zaman damgası olmadan buton her tıklandığında panel LostFocus ile kapanıp hemen ardından togglemenu mesajıyla tekrar açılırdı, yani ikinci tıklama hiçbir zaman kapatamazdı
    ULONGLONG m_menuSonKapanmaZamani = 0;

    static const int IzinPaneliGenisligi = 360; // İzin bildirim panelinin sabit genişliği
    static const int IzinPaneliYuksekligi = 150; // İzin bildirim panelinin sabit yüksekliği, içindeki metin/butonlar için yeterli pay bırakır

    static const int SitePaneliGenisligi = 300; // Kilit ikonu panelinin sabit genişliği
    static const int SitePaneliYuksekligi = 320; // Kilit ikonu panelinin azami yüksekliği, içindeki izin listesi uzunsa panel kendi içinde kaydırılır

    static const int SertifikaPaneliGenisligi = 380; // Sertifika hatası panelinin sabit genişliği
    static const int SertifikaPaneliYuksekligi = 340; // Sertifika hatası panelinin sabit yüksekliği

    std::vector<KapatilanSekmeBilgisi> m_kapatilanSekmeler; // En son kapatılan sekmelerin adreslerini tutan basit bir yığın, "Son Kapatılan Sekmeyi Aç" özelliği için kullanılır, vektörün SONU yığının tepesidir
    static const size_t KapatilanSekmeYigitiHedefBoyutu = 15; // Yığında aynı anda en fazla bu kadar kapatılmış sekme tutulur, daha eskiler atılır

    static const int DevToolsPaneliGenisligi = 460; // Gömülü DevTools panelinin sabit genişliği, gerçek Chrome DevTools penceresinin alışılmış genişliğine yakın tutulur
    static const int DevToolsUzaktanHataAyiklamaPortu = 9333; // WebView2'nin, bu panelin bağlanacağı Chrome DevTools Protokolü'nü bu yerel bağlantı noktasında sunması için ortam seçeneklerinde ayarlanan port
    bool m_devToolsAcikMi = false; // DevTools panelinin şu anda açık olup olmadığı, hem panel görünürlüğünü hem de içerik alanının daralıp daralmayacağını belirler
    // DevTools hedefini bulma isteği, arayüzün donmaması için ayrı bir iş parçacığında (thread) yapılır, sonuç hazır olunca bu özel pencere mesajıyla ana iş parçacığına geri bildirilir
    // Mesajın LPARAM'ı, bulunan DevTools arayüz adresini (veya hiçbir hedef bulunamadıysa boş bir metni) tutan, "new" ile oluşturulmuş bir std::wstring işaretçisidir, bu işaretçinin sahipliği mesajı İŞLEYEN tarafa geçer, o taraf işini bitirince "delete" etmek ZORUNDADIR
    static const UINT DevToolsHedefBulunduMesaji = WM_APP + 1;

    // Bu yapı, arka planda tamamlanan bir şifre sızıntı/zayıflık kontrolünün sonucunu taşır
    struct SizintiSonucu
    {
        int sekmeId = 0;
        std::wstring site;
        std::wstring kullaniciAdi;
        bool dogrulamaBasarisizMi = false; // Windows kimlik doğrulaması başarısız olduysa, diğer alanlar anlamsızdır
        bool zayifMi = false;
        bool sizdirilmisMi = false;
        int sizintiSayisi = 0;
    };
    // Sızıntı kontrolü isteği, arayüzün donmaması için ayrı bir iş parçacığında yapılır, sonuç hazır olunca bu özel pencere mesajıyla ana iş parçacığına geri bildirilir
    // Mesajın LPARAM'ı, "new" ile oluşturulmuş bir SizintiSonucu işaretçisidir, sahipliği mesajı işleyen tarafa geçer, o taraf işini bitirince "delete" etmek ZORUNDADIR
    static const UINT SizintiSonucuMesaji = WM_APP + 3;

    // Bu yapı, Chrome Web Mağazası adresinden indirilip paketi açılmaya çalışılan bir uzantının sonucunu taşır
    struct MagazaEklentiSonucu
    {
        int sekmeId = 0;
        bool basarili = false;
        std::wstring hataMesaji; // basarili false ise, kullanıcıya gösterilecek kısa açıklama
        std::wstring klasorYolu; // basarili ise, paketi açılmış uzantının klasör yolu; AddBrowserExtension bu klasörle UI iş parçacığında çağrılır
    };
    // CRX indirme/paket açma işlemi, arayüzün donmaması için ayrı bir iş parçacığında yapılır, sonuç hazır olunca bu özel pencere mesajıyla ana iş parçacığına geri bildirilir
    // Mesajın LPARAM'ı, "new" ile oluşturulmuş bir MagazaEklentiSonucu işaretçisidir, sahipliği mesajı işleyen tarafa geçer, o taraf işini bitirince "delete" etmek ZORUNDADIR
    static const UINT MagazaEklentiSonucuMesaji = WM_APP + 4;

    ULONGLONG m_baslangicZamani = 0; // Programın açıldığı an, GetTickCount64() değeri olarak tutulur, Ayarlar sayfasındaki Hakkında bölümünde çalışma süresini göstermek için kullanılır

    static const int ToolbarYuksekligi = 96; // Pencerenin üstünde toolbar arayüzüne (sekme şeridi ve navigasyon çubuğu) ayrılan sabit piksel yüksekliği
    static const int YerImiCubuguYuksekligi = 28; // Yer imi çubuğu açıksa, toolbar'ın altına eklenen ekstra piksel yüksekliği

    // Bu fonksiyon, güncel ayarlara göre (yer imi çubuğu açık mı) toolbar'ın gerçekten kapladığı TOPLAM yüksekliği hesaplar, içerik alanının ve tüm panellerin Y konumu bu değere göre hesaplanır
    int ToplamToolbarYuksekligi();
    static const int SekmeUykuZamanlayiciId = 1; // Uykuya alma kontrolünü periyodik olarak tetikleyen Windows zamanlayıcısının kimlik numarası
};
