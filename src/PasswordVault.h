// Bu dosya, kullanıcının kaydettiği web sitesi şifrelerini, Windows'un kendi şifreleme sistemi (DPAPI) ile şifreleyip diske kaydeden ve gerektiğinde çözen fonksiyonların tanımını içerir.
// DPAPI, şifrelenen veriyi sadece aynı Windows kullanıcı hesabında çözülebilecek şekilde korur, bu yüzden şifreler başka bir bilgisayara kopyalansa bile orada okunamaz.
// Bu dosyadaki fonksiyonlar, Ayarlar sekmesindeki Şifreler bölümü tarafından çağrılır, şifrenin gerçek metnini görmeden önce GuvenlikDogrulama.h'deki Windows şifre doğrulamasının geçilmiş olması beklenir.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla şifre kaydını bir liste halinde tutmak için eklenir

// Bu yapı, kaydedilmiş tek bir şifreyi temsil eder, sifreliVeri alanı her zaman şifreli ve Base64 ile kodlanmış haldedir, düz metin şifre asla bu yapıda saklanmaz
struct SifreKaydi
{
    std::wstring site; // Bu şifrenin ait olduğu web sitesi, örneğin "example.com"
    std::wstring kullaniciAdi; // Bu sitede kullanılan kullanıcı adı veya e-posta
    std::wstring sifreliVeri; // Şifrenin, DPAPI ile şifrelenip Base64 ile kodlanmış hali
    std::wstring eklenmeZamani; // Bu kaydın eklendiği zaman
};

namespace SifreKasasi // Tüm şifre yöneticisi fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, verilen site, kullanıcı adı ve düz metin şifreyi alıp, şifreyi DPAPI ile şifreleyerek yeni bir kayıt olarak diske ekler
    void SifreEkle(const std::wstring& site, const std::wstring& kullaniciAdi, const std::wstring& duzSifre);

    // Bu fonksiyon, verilen site ve kullanıcı adına sahip kaydı siler
    void SifreSil(const std::wstring& site, const std::wstring& kullaniciAdi);

    // Bu fonksiyon, kayıtlı tüm şifreleri siler, veri temizleme panelinde "Şifreler" kategorisi seçildiğinde kullanılır
    void TumSifreleriSil();

    // Bu fonksiyon, kaydedilmiş tüm şifreleri, şifreli haliyle (çözülmeden) bir liste olarak döndürür, bu liste site ve kullanıcı adlarını güvenle gösterebilmek için kullanılır
    std::vector<SifreKaydi> SifreleriOku();

    // Bu fonksiyon, verilen şifreli ve Base64 kodlu bir veriyi, DPAPI ile çözüp düz metin şifreyi döndürür, çözme başarısız olursa boş bir metin döndürür
    std::wstring SifreyiCoz(const std::wstring& sifreliVeri);

    // Bu fonksiyon, verilen düz metin şifrenin zayıf olup olmadığını, tamamen yerel ve ağa hiçbir şey göndermeden kontrol eder: kısa uzunluk, tek tip karakter veya en yaygın bilinen şifrelerden biri olma durumlarına bakar
    bool SifreZayifMi(const std::wstring& duzSifre);

    // Bu fonksiyon, verilen düz metin şifrenin SHA-1 özetini, büyük harf onaltılık bir metin olarak döndürür, Have I Been Pwned'ın k-anonymity API'sine sadece bu özetin ilk 5 karakterini göndermek için kullanılır, şifrenin kendisi asla ağa gönderilmez
    std::wstring SifreSha1OzetiAl(const std::wstring& duzSifre);
}
