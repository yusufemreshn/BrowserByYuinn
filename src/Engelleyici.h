// Bu dosya, reklam engelleme ve izleyici (tracker) engelleme için kullanılan, küratörlü sabit domain listelerini ve bir adresin bu listelerden biriyle eşleşip eşleşmediğini kontrol eden fonksiyonu içerir.
// Bu, tam bir EasyList/EasyPrivacy ayrıştırıcısı DEĞİLDİR; yaygın bilinen birkaç yüz reklam/izleyici domainini kapsayan, orta düzeyde bir temel koruma sağlar.
// BrowserWindow.cpp, her sekmenin WebResourceRequested olayında bu fonksiyonu çağırıp eşleşen istekleri engeller.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <string> // Metin (std::wstring) türünü kullanmak için eklenir

namespace Engelleyici // Tüm engelleme fonksiyonları, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, verilen tam istek adresinin (URL), bilinen reklam sunucu domainlerinden birine ait olup olmadığını kontrol eder
    bool ReklamAdresiMi(const std::wstring& istekAdresi);

    // Bu fonksiyon, verilen tam istek adresinin, verilen izleyici engelleme seviyesine (temel/dengeli/siki) göre bilinen bir izleyici domainine ait olup olmadığını kontrol eder
    // seviye "kapali" ise her zaman false döner, "dengeli" ve "siki" seviyeleri, kendinden önceki seviyenin tüm domainlerini de kapsar (birikimli)
    bool IzleyiciAdresiMi(const std::wstring& istekAdresi, const std::wstring& seviye);

    // Bu fonksiyon, verilen bir gezinme adresinin kimlik avı/kötü amaçlı site şüphesi taşıyan bazı yaygın teknik desenlerden birine uyup uymadığını kontrol eder
    // Bu, güncel bir kara liste servisi (örn. Google Safe Browsing) DEĞİLDİR, sadece ağa hiç çıkmadan tamamen yerel olarak çalışan, iki evrensel ipucuna bakan basit bir sezgiseldir:
    // (1) Alan adı yerine çıplak bir IP adresi kullanılması (meşru siteler neredeyse hiç bunu yapmaz), (2) Punycode (xn--) kodlu, görsel olarak taklit amaçlı kullanılabilecek bir alan adı
    bool SupheliAdresMi(const std::wstring& adres);
}
