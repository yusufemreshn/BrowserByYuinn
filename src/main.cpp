// Bu dosya, tarayıcı programının başlangıç noktasıdır, yani Windows işletim sistemi programı çalıştırdığında ilk çalışan kod buradadır.
// Bu dosyanın görevi artık çok küçüktür, çünkü pencereyi açma ve WebView2'yi yönetme işinin tamamı BrowserWindow sınıfına taşınmıştır.
// Bu dosya, sadece bir BrowserWindow nesnesi oluşturur, bu nesnenin penceresini açar, ve program kapanana kadar mesajları dinleyen döngüyü çalıştırır.
// Bu dosya, BrowserWindow.h dosyasına bağlıdır, çünkü asıl işi yapan sınıf oradadır.

#include <windows.h> // Mesaj döngüsü için gereken Windows türlerini ve fonksiyonlarını kullanmak için eklenir
#include "BrowserWindow.h" // Pencere ve WebView2 mantığını içeren BrowserWindow sınıfını kullanmak için eklenir
#include "VeriYolu.h" // Komut satırından okunan profil adını ayarlamak için eklenir

// Bu fonksiyon, programın ham komut satırı metninden "--profil=<ad>" argümanını arar, bulursa profil adını döndürür, bulamazsa boş metin döndürür
// Profil değiştirme özelliği (Ayarlar sayfası), yeni bir pencere açarken programı bu argümanla yeniden başlatır
static std::wstring KomutSatirindanProfilAdiniBul(PWSTR komutSatiri)
{
    std::wstring metin = komutSatiri ? komutSatiri : L"";
    const std::wstring onek = L"--profil=";
    size_t konum = metin.find(onek);
    if (konum == std::wstring::npos) { return L""; }
    konum += onek.size();
    size_t bitis = metin.find(L' ', konum); // Profil adı, bir sonraki boşluğa kadar (veya metnin sonuna kadar) sürer, profil adlarında boşluk olmasına izin verilmez
    return (bitis == std::wstring::npos) ? metin.substr(konum) : metin.substr(konum, bitis - konum);
}

// Bu fonksiyon, bir Windows masaüstü programının asıl başlangıç fonksiyonudur, main fonksiyonunun Windows'taki karşılığıdır
int APIENTRY wWinMain(HINSTANCE uygulamaOrnegi, HINSTANCE oncekiOrnek, PWSTR komutSatiri, int pencereGosterimDurumu)
{
    AktifProfilAdiniAyarla(KomutSatirindanProfilAdiniBul(komutSatiri)); // Herhangi bir veri okunmadan/yazılmadan ÖNCE, bu programın hangi profille çalışacağı belirlenir

    BrowserWindow tarayiciPenceresi; // Tarayıcının ana penceresini ve WebView2'lerini yönetecek nesne oluşturulur

    if (!tarayiciPenceresi.Create(uygulamaOrnegi)) // Pencere ve WebView2 kurulumu başlatılır, eğer pencere hiç açılamazsa
    {
        return 0; // Pencere açılamadığı için programın devam etmesinin bir anlamı yoktur
    }

    MSG mesaj = {}; // Gelen pencere mesajlarını tutacak boş bir yapı oluşturulur
    // Program kapatılana kadar sürekli olarak gelen mesajları dinleyen döngü, bir masaüstü programının kalbi burasıdır
    while (GetMessage(&mesaj, nullptr, 0, 0))
    {
        TranslateMessage(&mesaj); // Klavye tuş mesajlarını daha kullanılabilir bir hale çevirir
        DispatchMessage(&mesaj); // Mesajı, ilgili pencerenin mesaj işleyici fonksiyonuna gönderir
    }

    return 0; // Mesaj döngüsü bittiğinde, yani program kapandığında, programdan başarıyla çıkılır
}
