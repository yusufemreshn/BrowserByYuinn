// Bu dosya, ham (binary) veriyi metne, ve bu metni tekrar ham veriye çeviren Base64 kodlama/çözme yardımcı fonksiyonlarını içerir.
// Bu fonksiyonlar, PasswordVault.cpp tarafından, Windows'un DPAPI ile şifrelediği ham baytları, bir JSON dosyasının içine düz metin olarak yazılabilecek hale getirmek için kullanılır.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Ham baytları bir liste halinde tutmak için eklenir
#include <cstdint> // Sabit boyutlu sayı türlerini (uint8_t gibi) kullanmak için eklenir

// Bu fonksiyon, verilen ham baytları, Base64 alfabesini kullanarak bir metne çevirir
inline std::wstring Base64Kodla(const std::vector<uint8_t>& veri)
{
    static const wchar_t alfabe[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // Base64'ün kullandığı altmış dört karakterlik sabit alfabe

    std::wstring sonuc; // Kodlanmış sonucu biriktirecek boş bir metin oluşturulur
    size_t i = 0; // Veri içinde üçerli gruplar halinde ilerlemek için kullanılan sayaç

    while (i + 3 <= veri.size()) // Veri, tam üçer baytlık gruplar halinde işlenebildiği sürece
    {
        uint32_t uclu = (veri[i] << 16) | (veri[i + 1] << 8) | veri[i + 2]; // Üç bayt, tek bir yirmi dört bitlik sayıya birleştirilir
        sonuc += alfabe[(uclu >> 18) & 0x3F]; // Bu sayının ilk altı biti, alfabedeki karşılığına çevrilir
        sonuc += alfabe[(uclu >> 12) & 0x3F]; // İkinci altı bit, alfabedeki karşılığına çevrilir
        sonuc += alfabe[(uclu >> 6) & 0x3F]; // Üçüncü altı bit, alfabedeki karşılığına çevrilir
        sonuc += alfabe[uclu & 0x3F]; // Dördüncü altı bit, alfabedeki karşılığına çevrilir
        i += 3; // Bir sonraki üçlü gruba geçilir
    }

    size_t kalan = veri.size() - i; // Tam üçlü gruplara bölünemeyen, geriye kalan bayt sayısı hesaplanır
    if (kalan == 1) // Eğer geriye tek bir bayt kaldıysa
    {
        uint32_t tek = veri[i] << 16; // Kalan bayt, yirmi dört bitlik bir sayının en başına yerleştirilir
        sonuc += alfabe[(tek >> 18) & 0x3F]; // İlk altı bit çevrilir
        sonuc += alfabe[(tek >> 12) & 0x3F]; // İkinci altı bit çevrilir
        sonuc += L"=="; // Base64 standardına göre, eksik kalan iki karakterin yerine doldurma işareti konur
    }
    else if (kalan == 2) // Eğer geriye iki bayt kaldıysa
    {
        uint32_t iki = (veri[i] << 16) | (veri[i + 1] << 8); // Kalan iki bayt, yirmi dört bitlik bir sayının başına yerleştirilir
        sonuc += alfabe[(iki >> 18) & 0x3F]; // İlk altı bit çevrilir
        sonuc += alfabe[(iki >> 12) & 0x3F]; // İkinci altı bit çevrilir
        sonuc += alfabe[(iki >> 6) & 0x3F]; // Üçüncü altı bit çevrilir
        sonuc += L"="; // Base64 standardına göre, eksik kalan tek karakterin yerine doldurma işareti konur
    }

    return sonuc; // Tamamen kodlanmış metin döndürülür
}

// Bu fonksiyon, verilen bir karakterin Base64 alfabesindeki sıra numarasını (0 ile 63 arası) döndürür, karakter alfabede yoksa negatif bir değer döndürür
inline int Base64KarakterDegeri(wchar_t karakter)
{
    if (karakter >= L'A' && karakter <= L'Z') { return karakter - L'A'; } // Büyük harfler, alfabenin ilk yirmi altı sırasına karşılık gelir
    if (karakter >= L'a' && karakter <= L'z') { return karakter - L'a' + 26; } // Küçük harfler, alfabenin sonraki yirmi altı sırasına karşılık gelir
    if (karakter >= L'0' && karakter <= L'9') { return karakter - L'0' + 52; } // Rakamlar, alfabenin son on sırasına karşılık gelir
    if (karakter == L'+') { return 62; } // Artı işareti, altmış ikinci sıraya karşılık gelir
    if (karakter == L'/') { return 63; } // Bölü işareti, altmış üçüncü sıraya karşılık gelir
    return -1; // Tanınmayan bir karakterse, geçersiz olduğunu belirten negatif bir değer döndürülür
}

// Bu fonksiyon, Base64 ile kodlanmış bir metni, orijinal ham baytlara geri çevirir
inline std::vector<uint8_t> Base64Coz(const std::wstring& metin)
{
    std::vector<uint8_t> sonuc; // Çözülmüş ham baytları biriktirecek boş bir liste oluşturulur

    int biriktirilenBitler = 0; // Henüz tam bir bayt oluşturacak kadar biriktirilmemiş bitlerin değeri
    int bitSayisi = 0; // Şu ana kadar biriktirilen bit sayısı

    for (wchar_t karakter : metin) // Verilen metindeki her karakter sırayla gezilir
    {
        if (karakter == L'=' || karakter == L'\r' || karakter == L'\n') // Doldurma işareti veya satır sonu karakterleriyse
        {
            continue; // Bu karakter atlanır, çözme işlemine dahil edilmez
        }

        int deger = Base64KarakterDegeri(karakter); // Karakterin alfabedeki sıra numarası bulunur
        if (deger < 0) // Eğer karakter Base64 alfabesinde yoksa
        {
            continue; // Bu geçersiz karakter atlanır
        }

        biriktirilenBitler = (biriktirilenBitler << 6) | deger; // Bu karakterin altı biti, biriktirilen bitlerin sonuna eklenir
        bitSayisi += 6; // Biriktirilen bit sayısı altı artırılır

        if (bitSayisi >= 8) // Eğer artık tam bir bayt oluşturacak kadar bit birikmişse
        {
            bitSayisi -= 8; // Kullanılacak sekiz bit, biriktirilen bit sayısından düşülür
            sonuc.push_back(static_cast<uint8_t>((biriktirilenBitler >> bitSayisi) & 0xFF)); // En baştaki sekiz bit, bir bayt olarak sonuca eklenir
        }
    }

    return sonuc; // Çözülmüş ham baytlar döndürülür
}
