// Bu dosya, programın tüm kalıcı verilerinin (geçmiş, yer imleri, ayarlar, şifreler, günlükler) saklandığı ana klasörün yolunu bulan ortak bir yardımcı fonksiyon içerir.
// Storage.cpp, Logger.cpp, Settings.cpp ve PasswordVault.cpp gibi diskle konuşan her dosya, aynı klasörü kullanmak için bu fonksiyonu çağırır, böylece klasör yolu tek bir yerde tanımlanmış olur.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <windows.h> // Ortam değişkenini okumak ve klasör oluşturmak için Windows fonksiyonlarını kullanmak için eklenir
#include <string> // Metin (std::wstring) türünü kullanmak için eklenir

// Bu fonksiyon, programın şu anda hangi profille çalıştığını tutan paylaşılan metne bir referans döndürür
// "inline" bir fonksiyonun içindeki "static" yerel değişken, C++17'den itibaren TÜM derleme birimlerinde aynı tek örneği paylaşır, bu yüzden bu .h dosyasını içeren her .cpp dosyası aynı profil adını görür
// Boş bırakılırsa (varsayılan), eski tek-profilli kurulumlarla geriye dönük uyumluluk için hiçbir alt klasöre girilmez, doğrudan ana "tarayici" klasörü kullanılır
inline std::wstring& AktifProfilAdiReferansi()
{
    static std::wstring profilAdi;
    return profilAdi;
}

// Bu fonksiyon, programın hangi profille çalışacağını ayarlar, main.cpp tarafından komut satırı argümanı okunduktan hemen sonra, başka hiçbir veri okunmadan/yazılmadan önce çağrılmalıdır
inline void AktifProfilAdiniAyarla(const std::wstring& profilAdi)
{
    AktifProfilAdiReferansi() = profilAdi;
}

// Bu fonksiyon, profil kavramından tamamen bağımsız olarak, kullanıcının AppData\Roaming klasörünün altındaki ana "tarayici" klasörünün yolunu döndürür, klasör henüz yoksa önce oluşturur
// Profil listesini bulmak (profiller her zaman bu kök klasörün altındadır) gibi, "şu an hangi profildeyim" sorusundan bağımsız işler için kullanılır
inline std::wstring KokVeriKlasoruAl()
{
    wchar_t* appDataYolu = nullptr; // Kullanıcının AppData klasörünün yolunu tutacak işaretçi
    size_t uzunluk = 0; // AppData ortam değişkeninin uzunluğunu tutacak değişken
    _wdupenv_s(&appDataYolu, &uzunluk, L"APPDATA"); // Sistemdeki APPDATA ortam değişkeninin değeri okunur

    std::wstring klasorYolu = std::wstring(appDataYolu) + L"\\tarayici"; // AppData klasörünün altına, bu program için ayrılmış "tarayici" adlı bir alt klasör yolu oluşturulur
    free(appDataYolu); // Artık kullanılmayan bellek serbest bırakılır

    CreateDirectoryW(klasorYolu.c_str(), nullptr); // Bu klasör zaten varsa hiçbir şey yapmaz, yoksa oluşturur
    return klasorYolu;
}

// Bu fonksiyon, kullanıcının AppData\Roaming klasörünün altındaki "tarayici" adlı ana veri klasörünün (bir profil seçiliyse onun alt klasörünün) yolunu döndürür, klasör henüz yoksa önce oluşturur
inline std::wstring VeriKlasoruAl()
{
    std::wstring klasorYolu = KokVeriKlasoruAl();

    const std::wstring& profilAdi = AktifProfilAdiReferansi();
    if (!profilAdi.empty()) // Bir profil seçiliyse, veriler ana klasörün altındaki "profiller\<ad>" alt klasöründe tutulur, böylece her profilin geçmişi/şifreleri/ayarları birbirinden tamamen izole kalır
    {
        std::wstring profillerKlasoru = klasorYolu + L"\\profiller";
        CreateDirectoryW(profillerKlasoru.c_str(), nullptr);
        klasorYolu = profillerKlasoru + L"\\" + profilAdi;
        CreateDirectoryW(klasorYolu.c_str(), nullptr);
    }

    return klasorYolu; // Hazırlanan klasör yolu döndürülür
}
