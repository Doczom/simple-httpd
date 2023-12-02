format binary as "bin"
use32
org 0

macro table arg3, [arg1,arg2] {
    local ..x,..x_end
    forward
       dd ..x
    common
    local ..other
       dd ..other
       ; или size = ($ - старт) / 8
    forward
       ..x db ..x_end-..x - 1, arg1
       ..x_end db arg2, 0
    common
       ..other  dd 0
                db arg3, 0

}


table   'application/octet-stream'      ,\
        '.html', 'text/html'            ,\
        '.css', 'text/css'              ,\
        '.js', 'text/javascript'        ,\
        '.txt', 'text/plain'            ,\
        '.pdf', 'application/pdf'       ,\
        '.json', 'application/json'     ,\
        '.png', 'image/png'             ,\
        '.mp3', 'audio/mpeg'            ,\
        '.mp4', 'video/mp4'
