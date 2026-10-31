// Bu dosya, Engelleyici.h içinde tanımı yapılan fonksiyonların gerçek çalışma mantığını ve küratörlü domain listelerini içerir.

#include "Engelleyici.h" // Bu dosyanın, hangi fonksiyonların gövdesini yazdığını bilmesi için kendi başlık dosyası eklenir
#include <cwctype> // towlower fonksiyonunu kullanmak için eklenir

namespace Engelleyici
{
    // Bilinen reklam sunucu/reklam ağı domainleri, küçük ama yaygın kullanılan bir küme
    static const wchar_t* ReklamDomainleri[] = {
        L"doubleclick.net", L"googlesyndication.com", L"googleadservices.com", L"adservice.google.com",
        L"adnxs.com", L"advertising.com", L"adsrvr.org", L"adform.net", L"adroll.com", L"criteo.com",
        L"criteo.net", L"outbrain.com", L"taboola.com", L"pubmatic.com", L"rubiconproject.com",
        L"openx.net", L"smartadserver.com", L"media.net", L"bidswitch.net", L"casalemedia.com",
        L"contextweb.com", L"indexexchange.com", L"sovrn.com", L"gumgum.com", L"yieldmo.com",
        L"amazon-adsystem.com", L"moatads.com", L"serving-sys.com", L"2mdn.net", L"adtechus.com",
        L"zedo.com", L"revcontent.com", L"mgid.com", L"propellerads.com", L"popads.net", L"exoclick.com",
        L"adcolony.com", L"applovin.com", L"unityads.unity3d.com", L"vungle.com"
    };

    // İzleyici engelleme — Temel seviye: en yaygın bilinen çapraz-site izleme ve analitik domainleri
    static const wchar_t* IzleyiciTemel[] = {
        L"google-analytics.com", L"googletagmanager.com", L"googletagservices.com", L"scorecardresearch.com",
        L"quantserve.com", L"hotjar.com", L"mixpanel.com", L"segment.com", L"segment.io", L"amplitude.com",
        L"fullstory.com", L"crazyegg.com", L"mouseflow.com", L"chartbeat.com", L"newrelic.com"
    };

    // İzleyici engelleme — Dengeli seviye: Temel'e ek olarak sosyal medya izleme pikselleri ve reklam-dışı pazarlama izleyicileri
    static const wchar_t* IzleyiciDengeli[] = {
        L"facebook.net", L"connect.facebook.net", L"analytics.twitter.com", L"ads-twitter.com",
        L"analytics.tiktok.com", L"snap.licdn.com", L"px.ads.linkedin.com", L"bat.bing.com",
        L"pixel.wp.com", L"stats.wp.com", L"analytics.yahoo.com", L"demdex.net", L"everesttech.net",
        L"adsymptotic.com", L"bluekai.com"
    };

    // İzleyici engelleme — Sıkı seviye: Dengeli'ye ek olarak parmak izi (fingerprinting) ve daha agresif izleme altyapıları, bazı sitelerde işlevsellik bozulabilir
    static const wchar_t* IzleyiciSiki[] = {
        L"fingerprintjs.com", L"fpjs.io", L"iovation.com", L"threatmetrix.com", L"maxmind.com",
        L"clarity.ms", L"yandex.ru/metrika", L"mc.yandex.ru", L"matomo.cloud", L"piwik.pro",
        L"optimizely.com", L"adobedtm.com", L"omtrdc.net", L"krxd.net", L"tapad.com"
    };

    // Bu fonksiyon, verilen tam bir istek adresinden sadece ana bilgisayar (host) kısmını ayıklar, şema ve yoldan arındırır
    static std::wstring AdresinHostKismi(const std::wstring& istekAdresi)
    {
        size_t semaSonu = istekAdresi.find(L"://");
        size_t hostBaslangici = (semaSonu == std::wstring::npos) ? 0 : (semaSonu + 3);
        size_t hostBitisi = istekAdresi.find_first_of(L"/?#", hostBaslangici);
        std::wstring host = (hostBitisi == std::wstring::npos) ? istekAdresi.substr(hostBaslangici) : istekAdresi.substr(hostBaslangici, hostBitisi - hostBaslangici);

        std::wstring kucukHost;
        for (wchar_t karakter : host) { kucukHost += static_cast<wchar_t>(towlower(karakter)); }
        return kucukHost;
    }

    // Bu fonksiyon, verilen host'un, verilen domain listesindeki domainlerden biriyle AYNI olup olmadığını veya o domainin bir ALT DOMAİNİ olup olmadığını kontrol eder
    static bool HostListedeMi(const std::wstring& host, const wchar_t* const* liste, size_t sayac)
    {
        for (size_t i = 0; i < sayac; i++)
        {
            std::wstring domain = liste[i];
            if (host == domain) { return true; } // Tam eşleşme
            if (host.size() > domain.size() && host.compare(host.size() - domain.size(), domain.size(), domain) == 0 && host[host.size() - domain.size() - 1] == L'.') { return true; } // "xxx.domain.com" gibi bir alt domain
        }
        return false;
    }

    bool ReklamAdresiMi(const std::wstring& istekAdresi)
    {
        std::wstring host = AdresinHostKismi(istekAdresi);
        if (host.empty()) { return false; }
        return HostListedeMi(host, ReklamDomainleri, sizeof(ReklamDomainleri) / sizeof(ReklamDomainleri[0]));
    }

    bool IzleyiciAdresiMi(const std::wstring& istekAdresi, const std::wstring& seviye)
    {
        if (seviye == L"kapali" || seviye.empty()) { return false; }

        std::wstring host = AdresinHostKismi(istekAdresi);
        if (host.empty()) { return false; }

        if (HostListedeMi(host, IzleyiciTemel, sizeof(IzleyiciTemel) / sizeof(IzleyiciTemel[0]))) { return true; }
        if (seviye == L"temel") { return false; }

        if (HostListedeMi(host, IzleyiciDengeli, sizeof(IzleyiciDengeli) / sizeof(IzleyiciDengeli[0]))) { return true; }
        if (seviye == L"dengeli") { return false; }

        // "siki" seviyesi, Temel ve Dengeli'nin tüm domainlerini zaten kontrol etmiş durumda, şimdi kendine özel listesi kontrol edilir
        return HostListedeMi(host, IzleyiciSiki, sizeof(IzleyiciSiki) / sizeof(IzleyiciSiki[0]));
    }

    // Bu fonksiyon, verilen host'un sadece rakam ve noktalardan oluşan, çıplak bir IPv4 adresi (örn. "192.168.1.1") olup olmadığını kontrol eder
    static bool CiplakIpAdresiMi(const std::wstring& host)
    {
        if (host.empty()) { return false; }
        int noktaSayisi = 0;
        for (wchar_t karakter : host)
        {
            if (karakter == L'.') { noktaSayisi++; }
            else if (!iswdigit(karakter)) { return false; } // Rakam veya nokta dışında bir karakter varsa, bu bir alan adıdır, IP adresi değildir
        }
        return noktaSayisi == 3; // Geçerli bir IPv4 adresi tam olarak üç nokta içerir
    }

    bool SupheliAdresMi(const std::wstring& adres)
    {
        std::wstring host = AdresinHostKismi(adres);
        if (host.empty()) { return false; }

        if (CiplakIpAdresiMi(host)) { return true; } // Alan adı yerine çıplak IP adresi kullanılması, meşru sitelerde neredeyse hiç görülmez
        if (host.find(L"xn--") != std::wstring::npos) { return true; } // Punycode kodlu alan adları, görsel olarak tanıdık bir markayı taklit etmek için kullanılabilir (homograph saldırısı)

        return false;
    }
}
