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
    forward
       ..x db ..x_end-..x - 1, arg1
       ..x_end db arg2, 0
    common
       ..other  dd 0
                db arg3, 0

}


table   'application/octet-stream'                      ,\
        '.html',        'text/html'                     ,\
        '.htm',         'text/html'                     ,\
        '.css',         'text/css'                      ,\
        '.js',          'text/javascript'               ,\
        '.txt',         'text/plain; charset=utf-8'     ,\
        '.pdf',         'application/pdf'               ,\
        '.rtf',         'application/rtf'               ,\
        '.json',        'application/json'              ,\
        '.xml',         'application/xml'               ,\
        '.ics',         'text/calendar'                 ,\
        '.cvs',         'text/csv'                      ,\
        \
        '.gz',          'application/gzip'              ,\
        '.zip',         'application/zip'               ,\
        '.7z',          'application/x-7z-compressed'   ,\
        '.bz',          'application/x-bzip'            ,\
        '.bz2',         'application/x-bzip2'           ,\
        '.tar',         'application/x-tar'             ,\
        \
        '.woff',        'font/woff'                     ,\
        '.woff2',       'font/woff2'                    ,\
        '.tff',         'font/ttf'                      ,\
        '.otf',         'font/otf'                      ,\
        \
        '.mp3',         'audio/mpeg'                    ,\
        '.mid',         'audio/midi'                    ,\
        '.midi',        'audio/midi'                    ,\
        '.wav',         'audio/wav'                     ,\
        '.weba',        'audio/webm'                    ,\
        '.opus',        'audio/opus'                    ,\
        '.oga',         'audio/ogg'                     ,\
        \
        '.mp4',         'video/mp4'                     ,\
        '.avi',         'video/x-msvideo'               ,\
        '.mpeg',        'video/mpeg'                    ,\
        '.webm',        'video/webm'                    ,\
        '.ogv',         'video/ogg'                     ,\
        '.mkv',         'application/x-matroska'        ,\
        \
        '.png',         'image/png'                     ,\
        '.bmp',         'image/bmp'                     ,\
        '.gif',         'image/gif'                     ,\
        '.avif',        'image/avif'                    ,\
        '.webp',        'image/webp'                    ,\
        '.svg',         'image/svg+xml'                 ,\
        '.apng',        'image/apng'                    ,\
        '.tif',         'image/tiff'                    ,\
        '.tiff',        'image/tiff'                    ,\
        '.jpeg',        'image/jpeg'                    ,\
        '.jpg',         'image/jpeg'
