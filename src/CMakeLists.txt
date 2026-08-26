cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(WebStrada LANGUAGES CXX C)

find_package(LLVM REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXSLT REQUIRED libxslt)
pkg_check_modules(JSONC REQUIRED json-c)
pkg_check_modules(PCRE2 REQUIRED libpcre2-8)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(MARIADB libmariadb)
pkg_check_modules(POSTGRES libpq)
pkg_check_modules(MINIZIP minizip)
pkg_check_modules(ICUUC REQUIRED icu-uc)
pkg_check_modules(ICU18N REQUIRED icu-i18n)
if(NOT MINIZIP_FOUND)
    pkg_check_modules(MINIZIP libminizip)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    add_link_options($<$<CONFIG:RELEASE>:-s>)
    add_link_options($<$<CONFIG:MINSIZEREL>:-s>)
endif()

option(WEBSTRADA_USE_PCH "Use precompiled headers" ON)

# Precompiled headers are used for Release builds only. In debug builds a
# developer edits headers frequently, and editing any header listed in the
# PCH invalidates it and forces a full project rebuild -- the opposite of a
# fast debug edit-rebuild cycle. Release builds are rare, so the rebuild
# cost does not matter there.
set(WEBSTRADA_USE_PCH_IN_BUILD OFF)
if(WEBSTRADA_USE_PCH AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(WEBSTRADA_USE_PCH_IN_BUILD ON)
endif()


add_library(webstrada-core
    string.cpp    ../include/webstrada/string.h
    cfvariant.cpp ../include/webstrada/cfvariant.h
    config.cpp    ../include/webstrada/config.h
    server_stats.cpp ../include/webstrada/server_stats.h
    db/db.cpp     ../include/webstrada/db.h
    db/db_sqlite.cpp
    db/db_qoq.cpp
    core/core_interp.cpp
    core/core_udf.cpp
    core/core_component.cpp
    core/core_query.cpp
    core/core_xml.cpp
    core/core_response.cpp
    core/core_locale.cpp
    core/core_eh.cpp
    core/core_membermethods.cpp
    core/core_misc.cpp
    core/core_stubs.cpp
    core/core_stacktrace.cpp
    cftags/tag_image.cpp ../include/webstrada/cfimage.h
    cftags/common.cpp
    cftags/tag_application.cpp
    cftags/tag_content.cpp
    cftags/tag_flush.cpp
    cftags/tag_header.cpp
    cftags/tag_http.cpp
    cftags/tag_httpparam.cpp
    cftags/tag_include.cpp
    cftags/tag_custom.cpp
    cftags/tag_cache.cpp
    cftags/tag_cferror.cpp
    cftags/tag_location.cpp
    cftags/tag_loop.cpp
    cftags/tag_query.cpp
    cftags/tag_queryparam.cpp
    cftags/tag_insert.cpp
    cftags/tag_dbinfo.cpp
    cftags/tag_storedproc.cpp
    cftags/tag_silent.cpp
    cftags/tag_transaction.cpp
    cftags/tag_logging.cpp
    cftags/tag_login.cpp
    cftags/tag_xml.cpp
    cftags/tag_directory.cpp
    cftags/tag_file.cpp
    cftags/tag_execute.cpp
    cftags/tag_wddx.cpp
    cftags/tag_feed.cpp
    cftags/tag_ftp.cpp
    cftags/tag_mail.cpp
    cftags/tag_schedule.cpp
    cftags/tag_zip.cpp
    cftags/tag_param.cpp
    cftags/tag_weboutput.cpp
    cffunctions/common.cpp
    cffunctions/fn_abs.cpp
    cffunctions/fn_acos.cpp
    cffunctions/fn_ajax.cpp
    cffunctions/fn_arrayappend.cpp
    cffunctions/fn_arrayavg.cpp
    cffunctions/fn_arrayclear.cpp
    cffunctions/fn_arraycontains.cpp
    cffunctions/fn_arraycontainsnocase.cpp
    cffunctions/fn_arraydelete.cpp
    cffunctions/fn_arraydeleteat.cpp
    cffunctions/fn_arraydeletenocase.cpp
    cffunctions/fn_arrayeach.cpp
    cffunctions/fn_arrayfilter.cpp
    cffunctions/fn_arrayfind.cpp
    cffunctions/fn_arrayfindall.cpp
    cffunctions/fn_arrayfindallnocase.cpp
    cffunctions/fn_arrayfindnocase.cpp
    cffunctions/fn_arrayfirst.cpp
    cffunctions/fn_arrayinsertat.cpp
    cffunctions/fn_arrayisdefined.cpp
    cffunctions/fn_arrayisempty.cpp
    cffunctions/fn_arraylast.cpp
    cffunctions/fn_arraylen.cpp
    cffunctions/fn_arraymax.cpp
    cffunctions/fn_arraymin.cpp
    cffunctions/fn_arraynew.cpp
    cffunctions/fn_arrayprepend.cpp
    cffunctions/fn_arrayreduce.cpp
    cffunctions/fn_arrayresize.cpp
    cffunctions/fn_arrayset.cpp
    cffunctions/fn_arrayshift.cpp
    cffunctions/fn_arrayslice.cpp
    cffunctions/fn_arraysum.cpp
    cffunctions/fn_arrayswap.cpp
    cffunctions/fn_arraytolist.cpp
    cffunctions/fn_arraypop.cpp
    cffunctions/fn_asin.cpp
    cffunctions/fn_atan.cpp
    cffunctions/fn_atan2.cpp
    cffunctions/fn_binarydecode.cpp
    cffunctions/fn_binaryencode.cpp
    cffunctions/fn_callstackdump.cpp
    cffunctions/fn_callstackget.cpp
    cffunctions/fn_cacheinfo.cpp
    cffunctions/fn_cacheevict.cpp
    cffunctions/fn_cacheclear.cpp
    cffunctions/fn_candeserialize.cpp
    cffunctions/fn_canserialize.cpp
    cffunctions/fn_ceiling.cpp
    cffunctions/fn_cjustify.cpp
    cffunctions/fn_canonicalize.cpp
    cffunctions/fn_clientvars.cpp
    cffunctions/fn_compare.cpp
    cffunctions/fn_comparenocase.cpp
    cffunctions/fn_configget.cpp
    cffunctions/fn_configreset.cpp
    cffunctions/fn_configset.cpp
    cffunctions/fn_cos.cpp
    cffunctions/fn_createdate.cpp
    cffunctions/fn_createdatetime.cpp
    cffunctions/fn_createodbcdate.cpp
    cffunctions/fn_createodbcdatetime.cpp
    cffunctions/fn_createodbctime.cpp
    cffunctions/fn_createtime.cpp
    cffunctions/fn_createtimespan.cpp
    cffunctions/fn_csrf.cpp
    cffunctions/fn_cache.cpp
    cffunctions/fn_csv.cpp
    cffunctions/fn_dateadd.cpp
    cffunctions/fn_datecompare.cpp
    cffunctions/fn_dateconvert.cpp
    cffunctions/fn_datediff.cpp
    cffunctions/fn_dateformat.cpp
    cffunctions/fn_datepart.cpp
    cffunctions/fn_datetimeformat.cpp
    cffunctions/fn_datasourcetest.cpp
    cffunctions/fn_day.cpp
    cffunctions/fn_dayofweek.cpp
    cffunctions/fn_dayofweekasstring.cpp
    cffunctions/fn_dayofyear.cpp
    cffunctions/fn_daysinmonth.cpp
    cffunctions/fn_daysinyear.cpp
    cffunctions/fn_de.cpp
    cffunctions/fn_decimalformat.cpp
    cffunctions/fn_decodeforhtml.cpp
    cffunctions/fn_decrementvalue.cpp
    cffunctions/fn_decrypt.cpp
    cffunctions/fn_decryptbinary.cpp
    cffunctions/fn_deserialize.cpp
    cffunctions/fn_deserializejson.cpp
    cffunctions/fn_directorycopy.cpp
    cffunctions/fn_directorycreate.cpp
    cffunctions/fn_directorydelete.cpp
    cffunctions/fn_directoryexists.cpp
    cffunctions/fn_directorylist.cpp
    cffunctions/fn_directoryrename.cpp
    cffunctions/fn_dollarformat.cpp
    cffunctions/fn_duplicate.cpp
    cffunctions/fn_emit_writedump.cpp
    cffunctions/fn_encodeforcss.cpp
    cffunctions/fn_encodefordn.cpp
    cffunctions/fn_encodeforhtml.cpp
    cffunctions/fn_encodeforhtmlattribute.cpp
    cffunctions/fn_encodeforjavascript.cpp
    cffunctions/fn_encodeforldap.cpp
    cffunctions/fn_encodeforurl.cpp
    cffunctions/fn_encodeforxml.cpp
    cffunctions/fn_encodeforxmlattribute.cpp
    cffunctions/fn_encodeforxpath.cpp
    cffunctions/fn_encrypt.cpp
    cffunctions/fn_encryptbinary.cpp
    cffunctions/fn_esapi.cpp
    cffunctions/fn_exp.cpp
    cffunctions/fn_expandpath.cpp
    cffunctions/fn_extensions.cpp
    cffunctions/fn_fileclose.cpp
    cffunctions/fn_filecopy.cpp
    cffunctions/fn_filedelete.cpp
    cffunctions/fn_fileexists.cpp
    cffunctions/fn_filegetmimetype.cpp
    cffunctions/fn_fileiseof.cpp
    cffunctions/fn_filemove.cpp
    cffunctions/fn_fileopen.cpp
    cffunctions/fn_fileread.cpp
    cffunctions/fn_filereadbinary.cpp
    cffunctions/fn_filereadline.cpp
    cffunctions/fn_fileseek.cpp
    cffunctions/fn_filesetaccessmode.cpp
    cffunctions/fn_filesetattribute.cpp
    cffunctions/fn_filesetlastmodified.cpp
    cffunctions/fn_fileskipbytes.cpp
    cffunctions/fn_fileupload.cpp
    cffunctions/fn_fileuploadall.cpp
    cffunctions/fn_filewrite.cpp
    cffunctions/fn_filewriteline.cpp
    cffunctions/fn_find.cpp
    cffunctions/fn_findnocase.cpp
    cffunctions/fn_floor.cpp
    cffunctions/fn_generate3deskey.cpp
    cffunctions/fn_generatepbkdfkey.cpp
    cffunctions/fn_generatesecretkey.cpp
    cffunctions/fn_getfileinfo.cpp
    cffunctions/fn_getcontextroot.cpp
    cffunctions/fn_getcpuusage.cpp
    cffunctions/fn_getcspnonce.cpp
    cffunctions/fn_getencoding.cpp
    cffunctions/fn_getexception.cpp
    cffunctions/fn_getfunctionlist.cpp
    cffunctions/fn_gethttptimestring.cpp
    cffunctions/fn_getlocale.cpp
    cffunctions/fn_getlocaledisplayname.cpp
    cffunctions/fn_getlocalhostip.cpp
    cffunctions/fn_getmetadata.cpp
    cffunctions/fn_getmetricdata.cpp
    cffunctions/fn_getprofilesections.cpp
    cffunctions/fn_getprofilestring.cpp
    cffunctions/fn_getpropertyfile.cpp
    cffunctions/fn_getreadableimageformats.cpp
    cffunctions/fn_getspace.cpp
    cffunctions/fn_getsystemmemory.cpp
    cffunctions/fn_gettimezoneinfo.cpp
    cffunctions/fn_gettoken.cpp
    cffunctions/fn_getwriteableimageformats.cpp
    cffunctions/fn_hash.cpp
    cffunctions/fn_hmac.cpp
    cffunctions/fn_hour.cpp
    cffunctions/fn_htmlcodeformat.cpp
    cffunctions/fn_htmleditformat.cpp
    cffunctions/fn_imageaddborder.cpp
    cffunctions/fn_imageblur.cpp
    cffunctions/fn_imageclearrect.cpp
    cffunctions/fn_imageclone.cpp
    cffunctions/fn_imagecopy.cpp
    cffunctions/fn_imagecreatecaptcha.cpp
    cffunctions/fn_imagecrop.cpp
    cffunctions/fn_imagedrawarc.cpp
    cffunctions/fn_imagedrawbeveledrect.cpp
    cffunctions/fn_imagedrawcubiccurve.cpp
    cffunctions/fn_imagedrawline.cpp
    cffunctions/fn_imagedrawlines.cpp
    cffunctions/fn_imagedrawoval.cpp
    cffunctions/fn_imagedrawpoint.cpp
    cffunctions/fn_imagedrawquadraticcurve.cpp
    cffunctions/fn_imagedrawrect.cpp
    cffunctions/fn_imagedrawroundrect.cpp
    cffunctions/fn_imagedrawtext.cpp
    cffunctions/fn_imageflip.cpp
    cffunctions/fn_imagegetblob.cpp
    cffunctions/fn_imagegetbufferedimage.cpp
    cffunctions/fn_imagegetexifmetadata.cpp
    cffunctions/fn_imagegetexiftag.cpp
    cffunctions/fn_imagegetheight.cpp
    cffunctions/fn_imagegetiptcmetadata.cpp
    cffunctions/fn_imagegetiptctag.cpp
    cffunctions/fn_imagegetmetadata.cpp
    cffunctions/fn_imagegetwidth.cpp
    cffunctions/fn_imagegrayscale.cpp
    cffunctions/fn_imageinfo.cpp
    cffunctions/fn_imagemakecolortransparent.cpp
    cffunctions/fn_imagemaketranslucent.cpp
    cffunctions/fn_imagenegative.cpp
    cffunctions/fn_imagenew.cpp
    cffunctions/fn_imageoverlay.cpp
    cffunctions/fn_imagepaste.cpp
    cffunctions/fn_imageread.cpp
    cffunctions/fn_imagereadbase64.cpp
    cffunctions/fn_imageresize.cpp
    cffunctions/fn_imagerotate.cpp
    cffunctions/fn_imagerotatedrawingaxis.cpp
    cffunctions/fn_imagescaletofit.cpp
    cffunctions/fn_imagesetantialiasing.cpp
    cffunctions/fn_imagesetbackgroundcolor.cpp
    cffunctions/fn_imagesetdrawingcolor.cpp
    cffunctions/fn_imagesetdrawingstroke.cpp
    cffunctions/fn_imagesetdrawingtransparency.cpp
    cffunctions/fn_imagesharpen.cpp
    cffunctions/fn_imageshear.cpp
    cffunctions/fn_imagesheardrawingaxis.cpp
    cffunctions/fn_imagetranslate.cpp
    cffunctions/fn_imagetranslatedrawingaxis.cpp
    cffunctions/fn_imagewrite.cpp
    cffunctions/fn_imagewritebase64.cpp
    cffunctions/fn_imagexordrawingmode.cpp
    cffunctions/fn_incrementvalue.cpp
    cffunctions/fn_int.cpp
    cffunctions/fn_iif.cpp
    cffunctions/fn_invoke.cpp
    cffunctions/fn_isarray.cpp
    cffunctions/fn_isbinary.cpp
    cffunctions/fn_isboolean.cpp
    cffunctions/fn_isclosure.cpp
    cffunctions/fn_iscustomfunction.cpp
    cffunctions/fn_isdate.cpp
    cffunctions/fn_isdateobject.cpp
    cffunctions/fn_isfileobject.cpp
    cffunctions/fn_isimage.cpp
    cffunctions/fn_isimagefile.cpp
    cffunctions/fn_isjson.cpp
    cffunctions/fn_isddx.cpp
    cffunctions/fn_isdebugmode.cpp
    cffunctions/fn_isipv6.cpp
    cffunctions/fn_isleapyear.cpp
    cffunctions/fn_islocalhost.cpp
    cffunctions/fn_isnull.cpp
    cffunctions/fn_isonline.cpp
    cffunctions/fn_ispdf.cpp
    cffunctions/fn_isthreadinterrupted.cpp
    cffunctions/fn_isvalid.cpp
    cffunctions/fn_iswddx.cpp
    cffunctions/fn_isnumeric.cpp
    cffunctions/fn_isnumericdate.cpp
    cffunctions/fn_isobject.cpp
    cffunctions/fn_isquery.cpp
    cffunctions/fn_issimplevalue.cpp
    cffunctions/fn_isstruct.cpp
    cffunctions/fn_jsstringformat.cpp
    cffunctions/fn_lcase.cpp
    cffunctions/fn_left.cpp
    cffunctions/fn_len.cpp
    cffunctions/fn_listappend.cpp
    cffunctions/fn_listchangedelims.cpp
    cffunctions/fn_listcontains.cpp
    cffunctions/fn_listcontainsnocase.cpp
    cffunctions/fn_listdeleteat.cpp
    cffunctions/fn_listeach.cpp
    cffunctions/fn_listfilter.cpp
    cffunctions/fn_listfind.cpp
    cffunctions/fn_listfindnocase.cpp
    cffunctions/fn_listfirst.cpp
    cffunctions/fn_listgetat.cpp
    cffunctions/fn_listgetduplicates.cpp
    cffunctions/fn_listinsertat.cpp
    cffunctions/fn_listlast.cpp
    cffunctions/fn_listlen.cpp
    cffunctions/fn_listmap.cpp
    cffunctions/fn_listprepend.cpp
    cffunctions/fn_listqualify.cpp
    cffunctions/fn_listreduce.cpp
    cffunctions/fn_listremoveduplicates.cpp
    cffunctions/fn_listrest.cpp
    cffunctions/fn_listsetat.cpp
    cffunctions/fn_listsort.cpp
    cffunctions/fn_listtoarray.cpp
    cffunctions/fn_listvaluecount.cpp
    cffunctions/fn_listvaluecountnocase.cpp
    cffunctions/fn_ljustify.cpp
    cffunctions/fn_location.cpp
    cffunctions/fn_log.cpp
    cffunctions/fn_log10.cpp
    cffunctions/fn_lscurrencyformat.cpp
    cffunctions/fn_lsdateformat.cpp
    cffunctions/fn_lsdatetimeformat.cpp
    cffunctions/fn_lseurocurrencyformat.cpp
    cffunctions/fn_lsiscurrency.cpp
    cffunctions/fn_lsisdate.cpp
    cffunctions/fn_lsisnumeric.cpp
    cffunctions/fn_lsnumberformat.cpp
    cffunctions/fn_lsparsecurrency.cpp
    cffunctions/fn_lsparsedatetime.cpp
    cffunctions/fn_lsparseeurocurrency.cpp
    cffunctions/fn_lsparsenumber.cpp
    cffunctions/fn_lstimeformat.cpp
    cffunctions/fn_ltrim.cpp
    cffunctions/fn_max.cpp
    cffunctions/fn_mid.cpp
    cffunctions/fn_min.cpp
    cffunctions/fn_minute.cpp
    cffunctions/fn_month.cpp
    cffunctions/fn_monthasstring.cpp
    cffunctions/fn_now.cpp
    cffunctions/fn_numberformat.cpp
    cffunctions/fn_objectequals.cpp
    cffunctions/fn_objectsave.cpp
    cffunctions/fn_parsedatetime.cpp
    cffunctions/fn_pi.cpp
    cffunctions/fn_quarter.cpp
    cffunctions/fn_queryaddcolumn.cpp
    cffunctions/fn_queryaddrow.cpp
    cffunctions/fn_queryconvertforgrid.cpp
    cffunctions/fn_queryeach.cpp
    cffunctions/fn_queryexecute.cpp
    cffunctions/fn_queryfilter.cpp
    cffunctions/fn_querygetresult.cpp
    cffunctions/fn_querygetrow.cpp
    cffunctions/fn_querykeyexists.cpp
    cffunctions/fn_querymap.cpp
    cffunctions/fn_querynew.cpp
    cffunctions/fn_queryreduce.cpp
    cffunctions/fn_querysetcell.cpp
    cffunctions/fn_quotedvaluelist.cpp
    cffunctions/fn_rand.cpp
    cffunctions/fn_randomize.cpp
    cffunctions/fn_randrange.cpp
    cffunctions/fn_reescape.cpp
    cffunctions/fn_preservesinglequotes.cpp
    cffunctions/fn_refind.cpp
    cffunctions/fn_refindnocase.cpp
    cffunctions/fn_rematch.cpp
    cffunctions/fn_rematchnocase.cpp
    cffunctions/fn_removechars.cpp
    cffunctions/fn_repeatstring.cpp
    cffunctions/fn_replace.cpp
    cffunctions/fn_replacelist.cpp
    cffunctions/fn_replacenocase.cpp
    cffunctions/fn_rereplace.cpp
    cffunctions/fn_rereplacenocase.cpp
    cffunctions/fn_reverse.cpp
    cffunctions/fn_right.cpp
    cffunctions/fn_rjustify.cpp
    cffunctions/fn_round.cpp
    cffunctions/fn_rtrim.cpp
    cffunctions/fn_second.cpp
    cffunctions/fn_serialize.cpp
    cffunctions/fn_serializejson.cpp
    cffunctions/fn_serverinfo.cpp
    cffunctions/fn_setlocale.cpp
    cffunctions/fn_setprofilestring.cpp
    cffunctions/fn_setvariable.cpp
    cffunctions/fn_sgn.cpp
    cffunctions/fn_sin.cpp
    cffunctions/fn_sleep.cpp
    cffunctions/fn_spanexcluding.cpp
    cffunctions/fn_spanincluding.cpp
    cffunctions/fn_sqr.cpp
    cffunctions/fn_stripcr.cpp
    cffunctions/fn_structappend.cpp
    cffunctions/fn_structclear.cpp
    cffunctions/fn_structcopy.cpp
    cffunctions/fn_structcount.cpp
    cffunctions/fn_structdelete.cpp
    cffunctions/fn_structeach.cpp
    cffunctions/fn_structfilter.cpp
    cffunctions/fn_structfind.cpp
    cffunctions/fn_structfindkey.cpp
    cffunctions/fn_structfindvalue.cpp
    cffunctions/fn_structget.cpp
    cffunctions/fn_structgetmetadata.cpp
    cffunctions/fn_structinsert.cpp
    cffunctions/fn_structisempty.cpp
    cffunctions/fn_structkeyarray.cpp
    cffunctions/fn_structkeyexists.cpp
    cffunctions/fn_structkeylist.cpp
    cffunctions/fn_structmap.cpp
    cffunctions/fn_structnew.cpp
    cffunctions/fn_structreduce.cpp
    cffunctions/fn_structsetmetadata.cpp
    cffunctions/fn_structsort.cpp
    cffunctions/fn_structtosorted.cpp
    cffunctions/fn_structupdate.cpp
    cffunctions/fn_structvaluearray.cpp
    cffunctions/fn_tan.cpp
    cffunctions/fn_timeformat.cpp
    cffunctions/fn_tobase64.cpp
    cffunctions/fn_tobinary.cpp
    cffunctions/fn_toscript.cpp
    cffunctions/fn_tostring.cpp
    cffunctions/fn_trace.cpp
    cffunctions/fn_transaction.cpp
    cffunctions/fn_trim.cpp
    cffunctions/fn_ucase.cpp
    cffunctions/fn_urldecode.cpp
    cffunctions/fn_urlencodedformat.cpp
    cffunctions/fn_urlsessionformat.cpp
    cffunctions/fn_val.cpp
    cffunctions/fn_valuelist.cpp
    cffunctions/fn_week.cpp
    cffunctions/fn_wrap.cpp
    cffunctions/fn_writedump.cpp
    cffunctions/fn_xmlchildpos.cpp
    cffunctions/fn_xmlelemnew.cpp
    cffunctions/fn_xmlformat.cpp
    cffunctions/fn_xmlgetnodetype.cpp
    cffunctions/fn_xmlnew.cpp
    cffunctions/fn_xmlparse.cpp
    cffunctions/fn_xmlsearch.cpp
    cffunctions/fn_xmltransform.cpp
    cffunctions/fn_xmlvalidate.cpp
    cffunctions/fn_year.cpp
    cffunctions/fn_yesnoformat.cpp
    upload.cpp    ../include/webstrada/upload.h
    scope_store.cpp ../include/webstrada/scope_store.h
    cache_store.cpp ../include/webstrada/cache_store.h
)

# The MySQL/MariaDB backend is optional: it is only compiled when the
# libmariadb client library is available (pkg-config). Without it the MySQL
# driver is absent from the registry and every datasource falls back to SQLite.
if(MARIADB_FOUND)
    target_sources(webstrada-core PRIVATE db/db_mysql.cpp)
    target_compile_definitions(webstrada-core PUBLIC WEBSTRADA_HAVE_MYSQL=1)
endif()

# The PostgreSQL backend is optional: it is only compiled when the libpq client
# library is available (pkg-config). Without it the PostgreSQL driver is absent
# from the registry and every datasource falls back to SQLite.
if(POSTGRES_FOUND)
    target_sources(webstrada-core PRIVATE db/db_pgsql.cpp)
    target_compile_definitions(webstrada-core PUBLIC WEBSTRADA_HAVE_POSTGRES=1)
endif()

target_include_directories(webstrada-core PUBLIC
    ../include
    ${LIBXML2_INCLUDE_DIRS}
    ${LIBXSLT_INCLUDE_DIRS}
    ${JSONC_INCLUDE_DIRS}
    ${PCRE2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${MINIZIP_INCLUDE_DIRS}
    ${ICUUC_INCLUDE_DIRS}
    ${ICU18N_INCLUDE_DIRS}
)

if(MARIADB_FOUND)
    target_include_directories(webstrada-core PUBLIC ${MARIADB_INCLUDE_DIRS})
endif()

if(POSTGRES_FOUND)
    target_include_directories(webstrada-core PUBLIC ${POSTGRES_INCLUDE_DIRS})
endif()

if(WEBSTRADA_USE_PCH_IN_BUILD)
    target_precompile_headers(webstrada-core PRIVATE
        ${CMAKE_SOURCE_DIR}/include/webstrada/pch.h
    )
endif()
target_link_libraries(webstrada-core PUBLIC
    LibXml2::LibXml2
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    ${LIBXSLT_LIBRARIES}
    ${JSONC_LIBRARIES}
    ${PCRE2_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CURL_LIBRARIES}
    ${MINIZIP_LIBRARIES}
    ${ICUUC_LIBRARIES}
    ${ICU18N_LIBRARIES}
)

if(MARIADB_FOUND)
    target_link_libraries(webstrada-core PUBLIC ${MARIADB_LIBRARIES})
endif()

if(POSTGRES_FOUND)
    target_link_libraries(webstrada-core PUBLIC ${POSTGRES_LIBRARIES})
endif()

add_library(webstrada-compiler
    codegen/fn_precisionevaluate.cpp
    codegen/llvm_codegen.cpp ../include/webstrada/llvm_codegen.h
    codegen/codegen_tokens.cpp
    codegen/codegen_expr.cpp
    codegen/codegen_script.cpp
    codegen/codegen_tags.cpp
    codegen/llvm_compiler.cpp ../include/webstrada/llvm_codegen.h
    parser.cpp   ../include/webstrada/parser.h
    template_reader.cpp ../include/webstrada/template_reader.h
)

# The CFML definition lives in the project's definitions/ directory (it is
# the authoritative copy). The system textparser package ships a matching
# cfml_definition.json; this project's copy is kept in sync with it by
# copying the updated system file over the local one and regenerating the
# header with textparser_json2h.py. Notable fixes synced in: the operator-
# grammar fix adding the symbolic && and || tokens, the removal of
# Parenthesis/ArrayIndex/CodeBlock from the top-level startTokens (fixed
# silent dropping of bracketed text in tag bodies), `multiLine` on
# `CodeBlock` (multi-line { } script blocks), and `contextNestedTokens` on
# `LoopExpression` (parses #...# SharpExpression in cfloop bodies nested
# inside cfoutput/cfquery).
set(TEXTPARSER_DEFINITIONS_DIR ${CMAKE_SOURCE_DIR}/definitions)
set(DEFINITIONS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/../definitions)

file(MAKE_DIRECTORY ${DEFINITIONS_OUTPUT_DIR})

foreach(def_file cfml_definition.json)
    add_custom_command(
        OUTPUT ${DEFINITIONS_OUTPUT_DIR}/${def_file}.h
        COMMAND /usr/bin/textparser_json2h.py --no-native-regex ${DEFINITIONS_OUTPUT_DIR}/${def_file}
        DEPENDS ${DEFINITIONS_OUTPUT_DIR}/${def_file}
        COMMENT "Generating ${def_file}.h"
    )
    target_sources(webstrada-compiler PRIVATE ${DEFINITIONS_OUTPUT_DIR}/${def_file}.h)
endforeach()

target_include_directories(webstrada-compiler PRIVATE
    ${DEFINITIONS_OUTPUT_DIR}
)

target_include_directories(webstrada-compiler PRIVATE
    ../include
)

if(WEBSTRADA_USE_PCH_IN_BUILD)
    target_precompile_headers(webstrada-compiler PRIVATE
        ${CMAKE_SOURCE_DIR}/include/webstrada/pch_llvm.h
    )
endif()

target_link_libraries(webstrada-compiler
    webstrada-core
    textparser
    LLVM
)

add_executable(WebStrada
    main.cpp
    template_cache.cpp ../include/webstrada/template_cache.h
    appserver.cpp          ../include/webstrada/appserver.h
    worker.cpp             ../include/webstrada/worker.h
)

target_include_directories(WebStrada PRIVATE
    ../include
)

if(WEBSTRADA_USE_PCH_IN_BUILD)
    target_precompile_headers(WebStrada PRIVATE
        ${CMAKE_SOURCE_DIR}/include/webstrada/pch.h
    )
endif()

target_link_libraries(WebStrada
    webstrada-core
    webstrada-compiler
    fcgi
)

include(GNUInstallDirs)
install(TARGETS WebStrada
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
