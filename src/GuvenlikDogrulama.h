// Bu dosya, kullanıcıdan Windows şifresini isteyen küçük bir onay penceresi açan ve girilen şifrenin gerçekten doğru olup olmadığını Windows'a soran fonksiyonun tanımını içerir.
// Bu fonksiyon, şifre yöneticisindeki bir kayıtlı şifre görüntülenmeden veya kopyalanmadan hemen önce çağrılır, Chrome ve Edge'in kayıtlı şifreleri göstermeden önce yaptığı kimlik doğrulamasına benzer bir amaca hizmet eder.

#pragma once // Bu başlık dosyasının bir derleme biriminde yanlışlıkla birden fazla kez eklenmesini önler

#include <windows.h> // Pencere oluşturma fonksiyonlarını ve HWND türünü kullanmak için eklenir

namespace Guvenlik // Güvenlik doğrulama fonksiyonu, başka isimlerle çakışmaması için bu isim alanının içine konur
{
    // Bu fonksiyon, kullanıcıya Windows şifresini soran küçük bir pencere açar, pencere kapanana kadar bekler, girilen şifre doğruysa doğru değerini, kullanıcı iptal ederse veya şifre yanlışsa yanlış değerini döndürür
    bool WindowsSifresiDogrula(HWND sahipPencere);

    // Bu fonksiyon, "otomatik kilitlenme" (geçici doğrulama hafızası) özelliğini uygular: kullanıcı en son başarıyla doğrulandıysa ve üzerinden verilenKilitlenmeDakikasi dakikadan azı geçtiyse, tekrar Windows şifresi sormadan doğrudan true döndürür
    // Aksi halde (hiç doğrulanmadıysa, süre dolduysa, veya verilenKilitlenmeDakikasi sıfırsa yani kullanıcı bu kolaylığı kapattıysa) kullanıcıya pencereyi açıp gerçekten sorar, başarılı olursa bu anı bir sonraki çağrı için hatırlar
    bool WindowsSifresiniKilitlenmeSuresiyleDogrula(HWND sahipPencere, int kilitlenmeDakikasi);
}
