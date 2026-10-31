// Bu dosya, kullanıcının sitelere verdiği izin kararlarını (konum, kamera, mikrofon, bildirim, otomatik oynatma gibi) kalıcı olarak diske kaydeden ve okuyan fonksiyonların tanımını içerir.
// Bir site bu izinlerden birini ilk kez istediğinde, kullanıcı toolbar'daki bildirim çubuğundan "İzin Ver" veya "Engelle" seçer, bu karar burada saklanır, bir sonraki istekte aynı site için tekrar sorulmaz.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir
#include <vector> // Birden fazla izin kaydını bir liste halinde tutmak için eklenir

// Bu yapı, kaydedilmiş tek bir izin kararını temsil eder
struct IzinKaydi
{
    std::wstring kaynak; // İznin verildiği sitenin kaynağı (origin), örneğin "https://example.com"
    std::wstring tur; // İzin türü, örneğin "konum", "kamera", "mikrofon", "bildirim", "otomatikoynatma"
    bool izinVerildi = false; // Kullanıcının kararı: true ise izin verilmiş, false ise engellenmiştir
};

namespace IzinKasasi // Tüm izin fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, verilen kaynak ve izin türü için daha önce kaydedilmiş bir karar olup olmadığını arar
    // Karar bulunduysa true döndürür ve izinVerildi parametresini doldurur, bulunamadıysa false döndürür ve izinVerildi parametresine dokunmaz
    bool KararBul(const std::wstring& kaynak, const std::wstring& tur, bool& izinVerildi);

    // Bu fonksiyon, verilen kaynak ve izin türü için bir karar kaydeder, aynı kaynak ve tür için zaten bir kayıt varsa onun üzerine yazılır
    void KararKaydet(const std::wstring& kaynak, const std::wstring& tur, bool izinVerildi);

    // Bu fonksiyon, verilen kaynak ve izin türüne ait kaydı siler, böylece bir sonraki istekte kullanıcıya tekrar sorulur
    void KararSil(const std::wstring& kaynak, const std::wstring& tur);

    // Bu fonksiyon, kaydedilmiş tüm izin kararlarını bir liste olarak döndürür, Ayarlar sayfasındaki izin yönetimi listesi için kullanılır
    std::vector<IzinKaydi> TumKararlariOku();
}
