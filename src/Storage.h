// Bu dosya, tarayıcının geçmiş (history.json) ve yer imleri (bookmarks.json) verilerini diske kaydetmesini ve diskten geri okumasını sağlayan fonksiyonların tanımını içerir.
// Bu dosyadaki fonksiyonlar, BrowserWindow sınıfı tarafından, bir sayfa ziyaret edildiğinde veya kullanıcı bir sayfayı yer imlerine eklediğinde çağrılır.
// Veriler, kullanıcının Windows kullanıcı profili altındaki AppData klasöründe, tarayici adlı bir klasörün içinde saklanır, böylece program nereden çalıştırılırsa çalıştırılsın veriler hep aynı yerde bulunur.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla girdiyi bir liste halinde tutmak için eklenir

// Bu yapı, geçmişteki veya yer imlerindeki tek bir kaydı temsil eder, yani bir web sayfasının adresini, başlığını ve kaydedildiği zamanı tutar
struct SayfaKaydi
{
    std::wstring adres; // Ziyaret edilen veya yer imine eklenen sayfanın web adresi
    std::wstring baslik; // Sayfanın başlığı, kullanıcıya listelerde gösterilecek metin
    std::wstring zaman; // Kaydın oluşturulduğu zamanın metin hali, ISO tarih formatında tutulur
    std::wstring klasor; // Sadece yer imlerinde kullanılır: bu yer iminin hangi (tek seviyeli) klasörde olduğu, boşsa klasörsüz (en üst seviye) demektir, geçmiş ve okuma listesi kayıtlarında kullanılmaz
};

namespace Depolama // Tüm depolama fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, verilen bir web sayfasını geçmiş listesine ekler, yani history.json dosyasının sonuna yeni bir kayıt ekler
    void GecmiseEkle(const std::wstring& adres, const std::wstring& baslik);

    // Bu fonksiyon, verilen bir web sayfasını yer imlerine ekler, yani bookmarks.json dosyasına yeni bir kayıt ekler, eğer bu adres zaten ekliyse tekrar eklenmez
    // klasor boş bırakılırsa, yer imi en üst seviyeye (klasörsüz) eklenir
    void YerImiEkle(const std::wstring& adres, const std::wstring& baslik, const std::wstring& klasor = L"");

    // Bu fonksiyon, verilen bir web sayfasını yer imlerinden kaldırır, yani bookmarks.json dosyasından bu adrese sahip kaydı siler
    void YerImiKaldir(const std::wstring& adres);

    // Bu fonksiyon, verilen bir web sayfasının şu anda yer imlerinde olup olmadığını kontrol eder
    bool YerImindeMi(const std::wstring& adres);

    // Bu fonksiyon, verilen adrese sahip yer iminin klasörünü değiştirir, böyle bir yer imi yoksa hiçbir şey yapmaz
    void YerImiKlasoruDegistir(const std::wstring& adres, const std::wstring& yeniKlasor);

    // Bu fonksiyon, yer imlerinin tamamını, verilen yeni adres sırasına göre yeniden diziler, Ayarlar sayfasındaki sürükle-bırak sıralaması tarafından kullanılır
    // yeniSiraAdresleri, mevcut tüm yer imi adreslerinin bir permütasyonu olmalıdır, değilse (eksik/fazla/tekrarlı bir adres varsa) güvenilmez kabul edilip hiçbir şey yapılmaz
    void YerImleriniSirala(const std::vector<std::wstring>& yeniSiraAdresleri);

    // Bu fonksiyon, yer imlerindeki tüm kayıtları bir liste olarak döndürür, dosya bozuksa veya hiç yoksa boş bir liste döner
    std::vector<SayfaKaydi> YerImleriniOku();

    // Bu fonksiyon, verilen bir web sayfasını okuma listesine ekler, eğer zaten ekliyse tekrar eklenmez
    void OkumaListesineEkle(const std::wstring& adres, const std::wstring& baslik);

    // Bu fonksiyon, verilen bir web sayfasını okuma listesinden kaldırır
    void OkumaListesindenKaldir(const std::wstring& adres);

    // Bu fonksiyon, verilen bir web sayfasının şu anda okuma listesinde olup olmadığını kontrol eder
    bool OkumaListesindeMi(const std::wstring& adres);

    // Bu fonksiyon, okuma listesindeki tüm kayıtları bir liste olarak döndürür, dosya bozuksa veya hiç yoksa boş bir liste döner
    std::vector<SayfaKaydi> OkumaListesiniOku();

    // Bu fonksiyon, geçmişteki tüm kayıtları bir liste olarak döndürür, dosya bozuksa veya hiç yoksa boş bir liste döner
    std::vector<SayfaKaydi> GecmisiOku();

    // Bu fonksiyon, geçmişteki kayıtları siler, baslangicZamaniIso boş bırakılırsa tüm geçmiş silinir, dolu verilirse sadece bu zamandan (ISO biçiminde, örn. "2026-10-24T00:00:00Z") sonraki kayıtlar silinir, daha eski kayıtlar korunur
    void GecmisiTemizle(const std::wstring& baslangicZamaniIso = L"");

    // Bu fonksiyon, GecmisiTemizle ile aynı şekilde çalışır, ancak adresi haricTutulanSiteler listesindeki herhangi bir siteyle başlayan kayıtları zaman aralığına bakmaksızın her zaman korur
    // haricTutulanSiteler, her biri bir site adresi (örn. "https://example.com") olan bir liste olmalıdır
    void GecmisiTemizleHaricTutarak(const std::wstring& baslangicZamaniIso, const std::vector<std::wstring>& haricTutulanSiteler);

    // Bu fonksiyon, geçmişten adres ve zamanı birebir eşleşen tek bir kaydı siler, Ayarlar sayfasındaki geçmiş listesinde her satırın yanındaki "Sil" butonu tarafından kullanılır
    void GecmistenKaydiSil(const std::wstring& adres, const std::wstring& zaman);

    // Bu fonksiyon, şu anda açık olan sekmelerin listesini, "önceki oturumu geri yükle" özelliği için diske kaydeder, sekmeler her değiştiğinde (açılınca/kapanınca) yeniden çağrılır, böylece dosya her zaman güncel kalır
    void OturumuKaydet(const std::vector<SayfaKaydi>& acikSekmeler);

    // Bu fonksiyon, en son kaydedilmiş oturumdaki sekmeleri bir liste olarak döndürür, dosya yoksa veya bozuksa boş bir liste döner
    std::vector<SayfaKaydi> OturumuOku();
}
