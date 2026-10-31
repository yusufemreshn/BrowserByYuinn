// Bu dosya, diske UTF-8 kodlamasıyla metin dosyası okuma ve yazma işlemleri için ortak yardımcı fonksiyonlar içerir.
// Standart kütüphanenin std::wifstream ve std::wofstream akışları, varsayılan olarak işletim sisteminin "C" yereline göre bayt dönüşümü yapar, bu da Türkçe karakterlerin (ı, ş, ğ, ü, ö, ç) bazen yanlış ya da hiç yazılamamasına sebep olur.
// Bu yüzden Storage.cpp, Logger.cpp, Settings.cpp ve PasswordVault.cpp, dosyaya doğrudan geniş metin akışıyla yazmak yerine, burada tanımlı fonksiyonlarla metni önce kendi elleriyle UTF-8 baytlara çevirip öyle yazar, okurken de aynı şekilde geri çevirir.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <windows.h> // UTF-8 ile geniş metin arasında dönüşüm yapan WideCharToMultiByte ve MultiByteToWideChar fonksiyonları için eklenir
#include <string> // Metin (std::wstring, std::string) türlerini kullanmak için eklenir
#include <fstream> // Dosyadan okuma ve dosyaya yazma işlemleri için eklenir

// Bu fonksiyon, verilen bir geniş metni (wstring), UTF-8 kodlamasında ham baytlara çevirir
inline std::string GenisMetniUtf8eCevir(const std::wstring& metin)
{
    if (metin.empty()) // Eğer metin boşsa
    {
        return std::string(); // Dönüştürülecek bir şey olmadığı için doğrudan boş bir sonuç döndürülür
    }
    int gerekliUzunluk = WideCharToMultiByte(CP_UTF8, 0, metin.c_str(), static_cast<int>(metin.size()), nullptr, 0, nullptr, nullptr); // Önce, sonucun kaç bayt tutacağı hesaplanır
    std::string sonuc(gerekliUzunluk, '\0'); // Hesaplanan uzunlukta, baştan doldurulmuş bir metin ayrılır
    WideCharToMultiByte(CP_UTF8, 0, metin.c_str(), static_cast<int>(metin.size()), sonuc.data(), gerekliUzunluk, nullptr, nullptr); // Asıl dönüşüm, ayrılan arabelleğin içine yapılır
    return sonuc; // UTF-8 baytlarından oluşan sonuç döndürülür
}

// Bu fonksiyon, UTF-8 kodlanmış ham baytları, bir geniş metne (wstring) çevirir, GenisMetniUtf8eCevir fonksiyonunun tam tersini yapar
inline std::wstring Utf8iGenisMetneCevir(const std::string& metin)
{
    if (metin.empty()) // Eğer metin boşsa
    {
        return std::wstring(); // Dönüştürülecek bir şey olmadığı için doğrudan boş bir sonuç döndürülür
    }
    int gerekliUzunluk = MultiByteToWideChar(CP_UTF8, 0, metin.c_str(), static_cast<int>(metin.size()), nullptr, 0); // Önce, sonucun kaç geniş karakter tutacağı hesaplanır
    std::wstring sonuc(gerekliUzunluk, L'\0'); // Hesaplanan uzunlukta, baştan doldurulmuş bir metin ayrılır
    MultiByteToWideChar(CP_UTF8, 0, metin.c_str(), static_cast<int>(metin.size()), sonuc.data(), gerekliUzunluk); // Asıl dönüşüm, ayrılan arabelleğin içine yapılır
    return sonuc; // Geniş karakterlerden oluşan sonuç döndürülür
}

// Bu fonksiyon, verilen yoldaki bir dosyayı UTF-8 olarak okuyup, içeriğini bir geniş metin (wstring) olarak döndürür, dosya açılamazsa boş metin döndürür
inline std::wstring Utf8DosyaOku(const std::wstring& dosyaYolu)
{
    std::ifstream dosya(dosyaYolu, std::ios::binary); // Dosya, hiçbir karakter dönüşümü yapmadan ham baytlar halinde okunmak üzere açılır
    if (!dosya.is_open()) // Eğer dosya açılamadıysa, örneğin henüz hiç oluşturulmadıysa
    {
        return L""; // Boş bir metin döndürülür
    }
    std::string hamIcerik((std::istreambuf_iterator<char>(dosya)), std::istreambuf_iterator<char>()); // Dosyanın tüm baytları, hiç değiştirilmeden bir metne toplanır
    return Utf8iGenisMetneCevir(hamIcerik); // Toplanan UTF-8 baytlar, geniş metne çevrilip döndürülür
}

// Bu fonksiyon, verilen geniş metni, UTF-8 kodlamasıyla verilen dosya yoluna yazar, ekle parametresi true verilirse dosyanın mevcut içeriğinin sonuna eklenir, aksi halde dosyanın önceki içeriği silinir
inline void Utf8DosyaYaz(const std::wstring& dosyaYolu, const std::wstring& metin, bool ekle = false)
{
    std::ofstream dosya(dosyaYolu, std::ios::binary | (ekle ? std::ios::app : std::ios::trunc)); // Dosya, hiçbir karakter dönüşümü yapmadan ham baytlar halinde yazılmak üzere açılır
    if (dosya.is_open()) // Eğer dosya başarıyla açılabildiyse
    {
        std::string utf8Icerik = GenisMetniUtf8eCevir(metin); // Yazılacak geniş metin, önce UTF-8 baytlarına çevrilir
        dosya.write(utf8Icerik.data(), static_cast<std::streamsize>(utf8Icerik.size())); // Dönüştürülen baytlar, hiçbir ek dönüşüm yapılmadan doğrudan dosyaya yazılır
    }
}
