// Bu dosya, tarayıcıda oluşan hataların kategori ve numara içeren bir koda bağlanıp diske kaydedilmesini sağlayan fonksiyonların tanımını içerir.
// Her hataya, örneğin NET-1001 gibi sabit bir kod verilir, bu kodun ne anlama geldiği KoduAciklamayaCevir fonksiyonunun içindeki gömülü sözlükte tutulur.
// Bu dosyadaki fonksiyonlar, BrowserWindow sınıfı tarafından, bir sekme bir sayfayı yükleyemediğinde veya bir sekmenin içeriği beklenmedik şekilde kapandığında çağrılır.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla günlük girdisini bir liste halinde tutmak için eklenir

// Bu yapı, günlük dosyasındaki tek bir hata kaydını temsil eder, Ayarlar sayfasındaki Hata Günlüğü bölümünde listelenirken kullanılır
struct HataGirdisi
{
    std::wstring tarih; // Bu hatanın kaydedildiği günün tarihi (dosya adından elde edilir)
    std::wstring saat; // Bu hatanın kaydedildiği saat
    std::wstring kod; // Bu hataya verilen kategori ve numara içeren kod
    std::wstring mesaj; // Bu hatayla ilgili ek bilgi (örneğin başarısız olan adres)
};

namespace Gunluk // Tüm günlükleme fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, verilen hata kodunu ve mesajını, kullanıcının bilgisayarındaki günlük dosyasına bir satır olarak ekler, böylece daha sonra incelenebilir
    void HataKaydet(const std::wstring& kod, const std::wstring& mesaj);

    // Bu fonksiyon, verilen bir hata koduna karşılık gelen kısa, kullanıcının anlayabileceği açıklamayı döndürür, kod tanınmıyorsa genel bir açıklama döner
    std::wstring KoduAciklamayaCevir(const std::wstring& kod);

    // Bu fonksiyon, en güncel günlük dosyalarından, en yeniden en eskiye doğru sıralanmış en fazla "azami" sayıda hata kaydını okuyup döndürür, Ayarlar sayfasındaki Hata Günlüğü listesini doldurmak için kullanılır
    std::vector<HataGirdisi> SonHatalariOku(int azami);
}
