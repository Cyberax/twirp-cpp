package twirpcpp

import (
	pgs "github.com/lyft/protoc-gen-star"
	pgsgo "github.com/lyft/protoc-gen-star/lang/go"
	"strings"
	"text/template"
)

type Module struct {
	*pgs.ModuleBase
	ctx pgsgo.Context
}

var _ pgs.Module = (*Module)(nil)

func Validator() pgs.Module { return &Module{ModuleBase: &pgs.ModuleBase{}} }

func (m *Module) InitContext(ctx pgs.BuildContext) {
	m.ModuleBase.InitContext(ctx)
	m.ctx = pgsgo.InitContext(ctx.Parameters())
}

func (m *Module) Name() string { return "twirpcpp" }

type TemplateData struct {
	pgs.File
	Namespace string
	Services  []pgs.Service
	FileName  string
}

func (m *Module) Execute(targets map[string]pgs.File, _ map[string]pgs.Package) []pgs.Artifact {

	fns := pgsgo.InitContext(m.Parameters())
	funcs := map[string]interface{}{
		"cmt":         pgs.C80,
		"name":        fns.Name,
		"pkg":         fns.PackageName,
		"typ":         fns.Type,
		"MakeComment": makeComment,
		"CppName":     cppName,
	}

	cppCliHeader := template.New("go")
	cppCliHeader.Funcs(funcs)
	template.Must(cppCliHeader.Parse(cppClientHeaderTpl))

	cppCliSrc := template.New("go")
	cppCliSrc.Funcs(funcs)
	template.Must(cppCliSrc.Parse(cppClientSrcTpl))

	cppSrvHeader := template.New("go")
	cppSrvHeader.Funcs(funcs)
	template.Must(cppSrvHeader.Parse(cppServerHeaderTpl))

	cppSrvSrc := template.New("go")
	cppSrvSrc.Funcs(funcs)
	template.Must(cppSrvSrc.Parse(cppServerSrcTpl))

	for _, f := range targets {
		m.Push(f.Name().String())

		//nsp := m.computeNamespace(f)
		fname := computeFilename(f)
		td := TemplateData{
			File:      f,
			Namespace: cppName(f.File()),
			FileName:  fname,
			Services:  f.Services(),
		}
		m.AddGeneratorTemplateFile(FilePathFor(f, m.ctx, "_client.hpp"), cppCliHeader, td)
		m.AddGeneratorTemplateFile(FilePathFor(f, m.ctx, "_client.cpp"), cppCliSrc, td)

		m.AddGeneratorTemplateFile(FilePathFor(f, m.ctx, "_server.hpp"), cppSrvHeader, td)
		m.AddGeneratorTemplateFile(FilePathFor(f, m.ctx, "_server.cpp"), cppSrvSrc, td)

		m.Pop()
	}

	return m.Artifacts()
}

func FilePathFor(f pgs.File, ctx pgsgo.Context, suffix string) string {
	return f.File().InputPath().SetExt(suffix).String()
}

func computeFilename(f pgs.File) string {
	return strings.TrimSuffix(f.Name().LowerSnakeCase().String(), "_proto")
}

func (m *Module) computeNamespace(f pgs.File) string {
	return f.Package().ProtoName().LowerSnakeCase().String()
}

func makeComment(linesOrString interface{}, numPad int) string {
	res := ""
	var lines []string
	if strLines, ok := linesOrString.([]string); ok {
		lines = strLines
	} else {
		lines = []string{linesOrString.(string)}
	}
	padding := strings.Repeat(" ", numPad)
	for _, line := range lines {
		parts := strings.Split(line, "\n")
		for i, ln := range parts {
			ln = strings.TrimRight(ln, "\n \t\r")
			if ln != "" {
				res += padding + "//" + ln + "\n"
			} else if i != len(parts)-1 {
				res += padding + "\n"
			}
		}
	}
	return res
}

func cppName(ent pgs.Entity) string {
	res := ""
	parts := strings.Split(ent.FullyQualifiedName(), ".")
	for _, c := range parts {
		if c == "" {
			continue
		}
		if res != "" {
			res += "::"
		}
		res += pgs.Name(c).String()
	}

	return res
}
