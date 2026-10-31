// Bu dosya, kullanıcının tarayıcı ayarlarını (varsayılan arama motoru, ana sayfa, tema tercihleri ve ilk açılış durumu) temsil eden veri yapısını ve bu ayarları diskten okuyup diske yazan fonksiyonların tanımını içerir.
// Bu dosyadaki fonksiyonlar, hem Ayarlar sekmesi arayüzü tarafından (kullanıcı bir ayarı değiştirdiğinde) hem de BrowserWindow sınıfı tarafından (örneğin ana sayfaya giderken hangi adresin kullanılacağını bilmek için) çağrılır.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir

// Bu yapı, kullanıcının tüm ayarlarını tek bir yerde toplar, settings.json dosyasının içeriğine karşılık gelir
struct AyarVerisi
{
    std::wstring aramaMotoru = L"google"; // Adres çubuğunda arama yapılırken kullanılacak arama motoru, "google", "bing" veya "duckduckgo" olabilir
    std::wstring anaSayfa = L"https://www.google.com"; // Ana sayfa butonuna ve yeni sekmelere basıldığında gidilecek adres
    std::wstring vurguRengi = L"#3b82f6"; // Arayüzde kullanılan vurgu (accent) rengi, onaltılık renk kodu olarak tutulur, kullanıcı serbestçe seçebilir
    std::wstring sekmeYogunlugu = L"normal"; // Sekme şeridinin görünümü, "kompakt" veya "normal" olabilir
    std::wstring yaziBoyutu = L"orta"; // Arayüz yazı boyutu, "kucuk", "orta" veya "buyuk" olabilir
    std::wstring temaModu = L"otomatik"; // Arayüzün açık mı koyu mu görüneceği, "otomatik" (Windows ayarını izler), "acik" veya "koyu" olabilir
    std::wstring koseYuvarlakligi = L"orta"; // Sekme ve kutuların köşe yuvarlaklığı, "kesik", "orta" veya "yuvarlak" olabilir
    bool ilkAcilisTamamlandiMi = false; // Kullanıcının karşılama (onboarding) ekranını görüp tamamladığı/atladığı, program bir daha asla bu ekranı göstermemesi için bunu hatırlar
    bool yerImiCubuguGoster = true; // Toolbar'ın altında, en üst seviye yer imlerini gösteren ince çubuğun görünüp görünmeyeceği
    int sifreKasasiKilitlenmeDakikasi = 5; // Bir şifre Windows kimlik doğrulamasıyla gösterildikten/doldurulduktan sonra, kaç dakika boyunca tekrar sorulmayacağı, 0 ise her zaman (hiç hatırlamadan) sorulur
    std::wstring yeniSekmeArkaPlanYolu; // Kullanıcının hızlı erişim sayfası için seçtiği özel arka plan görselinin diskteki tam yolu, boşsa varsayılan desenli arka plan kullanılır
    int uykuSekmeEsigiDakika = 2; // Bir sekmenin, aktif olmadığı için otomatik uykuya alınmadan önce bu kadar dakika hareketsiz kalması gerekir
    bool cikistaTemizle = false; // Açıksa, program penceresi kapatılırken aşağıdaki kategoriler otomatik olarak temizlenir
    std::wstring cikistaTemizleKategoriler; // "gecmis,cerez,onbellek" gibi virgülle ayrılmış kategori listesi, VeriTemizle fonksiyonundaki kategori adlarıyla aynı isimlendirmeyi kullanır
    std::wstring haricTutulanSiteler; // "|" ile ayrılmış site adresi listesi; bu adresler hem çıkışta otomatik temizlemeden hem de Ayarlar sayfasındaki geçmiş temizleme işleminden her zaman muaf tutulur
    bool indirmeKonumunuHerSeferSor = false; // Açıksa, her indirme başladığında kullanıcıya kaydedilecek klasör sorulur, kapalıyken varsayılan indirme klasörüne sessizce iner
    bool enerjiTasarrufuModu = false; // Açıksa, sekme uykuya alma eşiği daha agresif uygulanır ve arayüz animasyonları kısılır; dizüstü pil durumuna bağlı otomatik tetikleme BrowserWindow tarafında yapılır
    bool sonOturumuGeriYukle = false; // Açıksa, program açılışında hızlı erişim sayfası yerine, bir önceki kapanıştaki açık sekmeler geri yüklenir
    bool sizintiKontroluAcikMi = false; // Açıksa, şifre listesindeki "Sızıntı Kontrolü" butonu internete (Have I Been Pwned k-anonymity API'si) istek atabilir; gizlilik hassasiyeti nedeniyle varsayılan kapalıdır, şifrenin kendisi asla ağa gönderilmez, sadece SHA-1 özetinin ilk 5 karakteri gönderilir
    bool reklamEngellemeAcikMi = false; // Açıksa, küratörlü bir reklam sunucu domain listesiyle eşleşen ağ istekleri engellenir
    std::wstring izleyiciEngellemeSeviyesi = L"kapali"; // "kapali", "temel", "dengeli" veya "siki" olabilir, her seviye bir öncekinin tüm domainlerini de kapsar
    bool guvenliGezinmeAcikMi = true; // Açıksa, şüpheli desenlere (çıplak IP adresi, punycode alan adı) uyan sitelere gidilmeden önce bir uyarı ekranı gösterilir; tamamen yerel çalışır, ağa hiçbir şey göndermez, bu yüzden varsayılan açıktır
    std::wstring tarayiciDili = L"tr-TR"; // WebView2 ortamının dil kodu (BCP-47 biçiminde, örn. "tr-TR" veya "en-US"); site içeriği dili tercihini VE Chromium'un otomatik seçtiği yazım denetimi sözlüğünü belirler, sadece program yeniden başlatıldığında etkili olur çünkü ortam sadece başlangıçta bir kez oluşturulur

    // Aşağıdaki sekiz kısayol, kullanıcı tarafından Ayarlar sayfasından özelleştirilebilir, her biri Ctrl ile birlikte (Shift olmadan) basılacak harfi/tuşu tutar
    // Diğer tüm klavye kısayolları (Ctrl+Shift+... olanlar, yakınlaştırma gibi) kasıtlı olarak sabit tutuldu, hem karmaşıklığı sınırlamak hem de çatışma riskini azaltmak için
    wchar_t kisayolYeniSekme = L'T';
    wchar_t kisayolSekmeKapat = L'W';
    wchar_t kisayolCogalt = L'D';
    wchar_t kisayolYazdir = L'P';
    wchar_t kisayolSayfadaBul = L'F';
    wchar_t kisayolYenile = L'R';
    wchar_t kisayolAdres = L'L';
};

namespace Ayarlar // Tüm ayar fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, diskteki settings.json dosyasını okuyup bir AyarVerisi nesnesi olarak döndürür, dosya yoksa veya bozuksa varsayılan ayarları döndürür
    AyarVerisi Oku();

    // Bu fonksiyon, verilen ayarları settings.json dosyasına kaydeder
    void Kaydet(const AyarVerisi& ayarlar);
}
