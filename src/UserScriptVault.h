// Bu dosya, kullanıcının kendi yazdığı/eklediği küçük JavaScript betiklerini (userscript) diske kaydeden ve okuyan fonksiyonların tanımını içerir.
// Bu betikler, AddScriptToExecuteOnDocumentCreated üzerinden eşleşen sitelerde otomatik olarak çalıştırılır, tam bir eklenti mağazası değildir ama basit kişisel otomasyonlar (örneğin bir siteyi otomatik karartma, belirli öğeleri gizleme) için yeterlidir.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla betiği bir liste halinde tutmak için eklenir

// Bu yapı, kaydedilmiş tek bir kullanıcı betiğini temsil eder
struct KullaniciBetigi
{
    std::wstring id; // Bu betiğe ait, programın kendi içinde ürettiği benzersiz kimlik (metin olarak, zaman damgasından türetilir)
    std::wstring ad; // Betiğin kullanıcıya gösterilen adı
    std::wstring desen; // Betiğin hangi sitelerde çalışacağını belirten basit joker karakterli (*) desen, örneğin "*://*.example.com/*"
    std::wstring kod; // Betiğin kendi JavaScript kodu
    bool etkinMi = true; // Betiğin şu anda çalışmasının (enabled) açık mı kapalı mı olduğu
};

namespace BetikKasasi // Tüm kullanıcı betiği fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, verilen ad, desen ve koddan yeni bir kullanıcı betiği oluşturup diske ekler
    void BetikEkle(const std::wstring& ad, const std::wstring& desen, const std::wstring& kod);

    // Bu fonksiyon, verilen kimlikli betiği siler
    void BetikSil(const std::wstring& id);

    // Bu fonksiyon, verilen kimlikli betiğin etkin/kapalı durumunu değiştirir
    void BetikEtkinligiDegistir(const std::wstring& id, bool etkinMi);

    // Bu fonksiyon, kaydedilmiş tüm kullanıcı betiklerini bir liste olarak döndürür, dosya bozuksa veya hiç yoksa boş bir liste döndürür
    std::vector<KullaniciBetigi> BetikleriOku();
}
