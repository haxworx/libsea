#define UNUSED(x) (void)(x)
#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

#define PI 3.14159265358979323846
#define RAD(a)    ((a) * PI / 180)
#define DEG(a)    ((a) * 180 / PI)

// Largely inspired by "Zed's Awesome Debug Macros"
// From http://c.learncodethehardway.org/book/ex20.html

#define BEGIN do {
#define END   } while (0)

#define EPRINT(...) fprintf(stderr, __VA_ARGS__)
#ifdef _WIN32
#define EPRINT(...) BEGIN fprintf(stderr, __VA_ARGS__); fflush(stderr); END
#endif

#ifdef NDEBUG
#define DEBUG(M, ...)
#define DEBUG_LOC_FMT
#define DEBUG_LOC_VALS
#define DEBUG_LEVEL(X)
#else
#define DEBUG(M, ...)    EPRINT("[DEBUG] %s:%d %s: " M "\n", __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#define DEBUG_LOC_FMT  "%s:%d %s: "
#define DEBUG_LOC_VALS , __FILE__, __LINE__, __func__
#define DEBUG_LEVEL(X)   X
#endif

#define ERROR(M, ...)    EPRINT(DEBUG_LEVEL("[ERROR] ") DEBUG_LOC_FMT M "\n" DEBUG_LOC_VALS, ## __VA_ARGS__)
#define WARN(M, ...)     EPRINT(DEBUG_LEVEL("[WARN]  ") DEBUG_LOC_FMT M "\n" DEBUG_LOC_VALS, ## __VA_ARGS__)
#define INFO(M, ...)     EPRINT(DEBUG_LEVEL("[INFO]  ") DEBUG_LOC_FMT M "\n" DEBUG_LOC_VALS, ## __VA_ARGS__)
#define CHECK(A, M, ...) BEGIN if (!(A)) { ERROR(M, ## __VA_ARGS__); goto error; } END

#define C_STRERROR()     (errno == 0 ? "" : strerror(errno))
#define C_ERROR()        EPRINT(DEBUG_LEVEL("[ERROR] ") DEBUG_LOC_FMT "%s\n" DEBUG_LOC_VALS, C_STRERROR())
#define C_CHECK(A)       BEGIN if (!(A)) { C_ERROR(); errno = 0; goto error; } END

// Wrapper macros for libc

#define ALLOC(ptr, size) BEGIN void *tmp = realloc(ptr, size); if (size) C_CHECK(tmp); ptr = tmp; END
#define FCLOSE(fp)       BEGIN if (fp) { C_CHECK(fclose(fp) == 0); fp = NULL; } END

// Wrapper macros for UNIX

// Wrapper macros for SDL

#define SDL_STRERROR()             (*SDL_GetError() ? SDL_GetError() : "")
#define SDL_ERROR()                EPRINT(DEBUG_LEVEL("[ERROR] ") DEBUG_LOC_FMT "%s\n" DEBUG_LOC_VALS, SDL_STRERROR())
#define SDL_CHECK(A)               BEGIN if (!(A)) { SDL_ERROR(); SDL_ClearError(); goto error; } END

// Wrapper macros for OpenGL

#define GL_STRERROR(gl_error)      ((gl_error) == GL_NO_ERROR ? "" : (char *)gluErrorString(gl_error))
#define GL_ERROR(gl_error)         EPRINT(DEBUG_LEVEL("[ERROR] ") DEBUG_LOC_FMT "%s\n" DEBUG_LOC_VALS, GL_STRERROR(gl_error))
#define GL_CHECK()                 BEGIN GLenum gl_error = glGetError(); if (gl_error != GL_NO_ERROR) { GL_ERROR(gl_error); goto error; } END

// Wrapper macros for GLEW

#define GLEW_STRERROR(glew_error)  ((glew_error) == GLEW_OK ? "" : (char *)glewGetErrorString(glew_error))
#define GLEW_ERROR(glew_error)     EPRINT(DEBUG_LEVEL("[ERROR] ") DEBUG_LOC_FMT "%s\n" DEBUG_LOC_VALS, GLEW_STRERROR(glew_error))
#define GLEW_CHECK(glew_error)     BEGIN if (glew_error != GLEW_OK) { GLEW_ERROR(glew_error); goto error; } END

