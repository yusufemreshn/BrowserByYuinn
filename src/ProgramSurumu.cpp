// Bu dosya, ProgramSurumu::Al() fonksiyonunun gövdesini içerir, çalışan .exe'nin kendi gömülü sürüm bilgisini Windows'un sürüm bilgisi API'leriyle okur.
// Bu dosyada kullanılan GetFileVersionInfoSizeW, GetFileVersionInfoW ve VerQueryValueW fonksiyonları Version.dll içinde tanımlıdır, bu yüzden aşağıdaki pragma ile version.lib derleme zamanında bağlanır, projenin ayarlarına elle bir linker girdisi eklemeye gerek kalmaz.

#include "ProgramSurumu.h" // Bu dosyanın uyguladığı arayüzün bildirimi için eklenir

#include <Windows.h> // GetModuleFileNameW fonksiyonu ve temel Windows türleri için eklenir
#include <vector> // Sürüm bilgisi bloğunu okumak için kullanılan dinamik boyutlu arabellek (std::vector<BYTE>) için eklenir

#pragma comment(lib, "version.lib") // Version.dll'in import kütüphanesi, sürüm bilgisi fonksiyonlarını kullanabilmek için derleyiciye bağlanır

namespace ProgramSurumu
{
    std::wstring Al()
    {
        wchar_t exeYolu[MAX_PATH]; // Çalışan .exe'nin tam dosya yolu bu dizide tutulacak
        DWORD yolUzunlugu = GetModuleFileNameW(nullptr, exeYolu, MAX_PATH); // nullptr verildiğinde, çalışan sürecin kendi .exe dosyasının yolu döner
        if (yolUzunlugu == 0 || yolUzunlugu == MAX_PATH)
        {
            return L"0.0.0"; // Yol alınamazsa, onay karşılaştırmasında asla eşleşmeyecek güvenli bir varsayılan döndürülür
        }

        DWORD yokSayilanDeger = 0; // GetFileVersionInfoSizeW'nin kullanmadığı ama imzasının gerektirdiği bir çıktı parametresi
        DWORD arabellekBoyutu = GetFileVersionInfoSizeW(exeYolu, &yokSayilanDeger); // Sürüm bilgisi bloğunun kaç bayt olduğu öğrenilir
        if (arabellekBoyutu == 0)
        {
            return L"0.0.0"; // .exe'de sürüm bilgisi bloğu bulunamazsa (derleme ayarları bozulmuşsa) güvenli varsayılana dönülür
        }

        std::vector<BYTE> arabellek(arabellekBoyutu); // Sürüm bilgisi bloğu bu arabelleğe okunacak
        if (!GetFileVersionInfoW(exeYolu, 0, arabellekBoyutu, arabellek.data()))
        {
            return L"0.0.0";
        }

        UINT degerUzunlugu = 0; // VerQueryValueW'nin dolduracağı, bulunan metnin karakter sayısı
        LPVOID degerPtr = nullptr; // VerQueryValueW'nin arabellek içinde bulduğu metne işaret eden gösterici
        // "041f04b0" burada tarayici.rc'deki StringFileInfo alt bloğuyla birebir eşleşmelidir (Türkçe dil, Unicode kodlama)
        if (!VerQueryValueW(arabellek.data(), L"\\StringFileInfo\\041f04b0\\ProductVersion", &degerPtr, &degerUzunlugu) || degerPtr == nullptr)
        {
            return L"0.0.0"; // Beklenen dil/kodlama alt bloğu bulunamazsa güvenli varsayılana dönülür
        }

        return std::wstring(static_cast<wchar_t*>(degerPtr)); // Bulunan sürüm metni (örneğin "0.4.0") std::wstring olarak döndürülür
    }
}
