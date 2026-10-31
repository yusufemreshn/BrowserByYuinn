// Bu dosya, kullanıcının tek bir kişisel bilgi profilini (ad soyad, e-posta, telefon, adres) ve kayıtlı kredi kartlarını diske kaydeden ve okuyan fonksiyonların tanımını içerir.
// Kişisel bilgiler (ad, e-posta, telefon, adres) çok hassas sayılmadığı için düz metin olarak saklanır, tıpkı gerçek tarayıcıların otomatik doldurma verilerini sakladığı gibi.
// Kart numarası ise PasswordVault.h'deki şifrelerle aynı yöntemle (Windows DPAPI) şifrelenir, görüntülenmeden önce GuvenlikDogrulama.h'deki Windows şifre doğrulamasının geçilmiş olması beklenir.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla kart kaydını bir liste halinde tutmak için eklenir

// Bu yapı, kullanıcının tek bir kişisel bilgi profilini temsil eder, sayfalardaki form alanlarını otomatik doldurmak için kullanılmaz (bu, adres alanlarının çeşitliliği yüzünden güvenilir yapılamaz), sadece kopyala-yapıştır için saklanır
struct OtomatikDoldurmaProfili
{
    std::wstring adSoyad;
    std::wstring eposta;
    std::wstring telefon;
    std::wstring adres;
};

// Bu yapı, kaydedilmiş tek bir kredi kartını temsil eder, kartNumarasiSifreli alanı her zaman şifreli ve Base64 ile kodlanmış haldedir, düz metin kart numarası asla bu yapıda saklanmaz
struct KayitliKart
{
    std::wstring kartSahibi; // Kartın üzerinde yazan ad
    std::wstring kartNumarasiSifreli; // Kart numarasının, DPAPI ile şifrelenip Base64 ile kodlanmış hali
    std::wstring sonKullanma; // "AA/YY" biçiminde son kullanma tarihi, kart numarası kadar hassas sayılmadığı için düz metin tutulur
    std::wstring eklenmeZamani; // Bu kaydın eklendiği zaman
};

namespace OtomatikDoldurmaKasasi // Tüm otomatik doldurma kasası fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, kullanıcının kişisel bilgi profilini diske kaydeder, zaten var olan profilin üzerine yazar (tek bir profil desteklenir)
    void ProfilKaydet(const OtomatikDoldurmaProfili& profil);

    // Bu fonksiyon, diskteki kişisel bilgi profilini okur, hiç kaydedilmemişse tüm alanları boş bir profil döndürür
    OtomatikDoldurmaProfili ProfilOku();

    // Bu fonksiyon, verilen kart sahibi, düz metin kart numarası ve son kullanma tarihini alıp, kart numarasını DPAPI ile şifreleyerek yeni bir kayıt olarak diske ekler
    void KartEkle(const std::wstring& kartSahibi, const std::wstring& duzKartNumarasi, const std::wstring& sonKullanma);

    // Bu fonksiyon, verilen kart sahibi ve eklenme zamanına sahip (bu ikisi birlikte benzersiz bir anahtar oluşturur) kaydı siler
    void KartSil(const std::wstring& kartSahibi, const std::wstring& eklenmeZamani);

    // Bu fonksiyon, kayıtlı tüm kartları, şifreli haliyle (çözülmeden) bir liste olarak döndürür
    std::vector<KayitliKart> KartlariOku();

    // Bu fonksiyon, verilen şifreli ve Base64 kodlu bir kart numarasını, DPAPI ile çözüp düz metin halini döndürür, çözme başarısız olursa boş bir metin döndürür
    std::wstring KartNumarasiniCoz(const std::wstring& sifreliVeri);

    // Bu fonksiyon, kayıtlı profili ve tüm kartları siler, veri temizleme panelinde "Otomatik doldurma verileri" kategorisi seçildiğinde kullanılır
    void TumVeriyiSil();
}
