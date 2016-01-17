import os
import subprocess
import argparse

cmdLineParser = argparse.ArgumentParser(description="Simple wrapper for oscar-create")
cmdLineParser.add_argument('-f', action='store_true', help='Really execute the action', dest='force', default=False)
cmdLineParser.add_argument('-ckv', action='store_true', help='Create kvs', dest='ckv', required=False)
cmdLineParser.add_argument('-cs', action='store_true', help='Create searches', dest='cs', required=False)
cmdLineParser.add_argument('-w', help='Work files: bw, cal, germany, europe, planet', dest='w', nargs='+', type=str)
cmdLineParser.add_argument('-st', help='Search type: geocell, items, geohitems', dest='st', nargs='+', type=str)
cmdLineParser.add_argument('-sb', help='Search base: everything, bykvall, bykv', dest='sb', nargs='+', type=str)
cmdLineParser.add_argument('-dt', help="Database type: everything, bykvall, bykv", dest='dt', nargs='+', type=str)
cmdLineParser.add_argument('-pa', help="Database add boundary info: pa nopa", dest='pa', nargs='+', type=str)
cmdLineParser.add_argument('-tt', help="Tag adding: none reduced all", dest='tt', nargs='+', type=str)
cmdLineParser.add_argument('-sn', help="Name searching: prefix suffix", dest='sn', nargs='+', type=str)

parsedArgs = cmdLineParser.parse_args()

#dry run
dryRun = not parsedArgs.force

#create kvs before
createKvs = parsedArgs.ckv

#paths
srcFilesDir = "sourcefiles"
configDir = "configs"
oscarCmd = "./oscar-create"
srcFiles = {
            "planet": "planet-latest.osm.pbf",
            "europe": "europe-latest.osm.pbf",
            "germany": "germany-latest.osm.pbf",
            "bw": "baden-wuerttemberg-latest.osm.pbf",
            "cal" : "california-latest.osm.pbf"
}
outDirPath = "dest"#no trailing slash

#src file dependend config
gtOpts = {
            "planet": "p=1000x1000x2x2x250",
            "europe": "p=500x500x2x2x250",
            "germany": "p=200x200x2x2x250",
            "bw": "p=100x100x2x2x250",
            "cal": "p=100x100x2x2x250"
}

#searchTypes to create
workSearchTypes = parsedArgs.st
if (workSearchTypes is None):
    workSearchTypes = ["geocell", "items", "geohitems"]


#files to work upon
work = parsedArgs.w
if (work is None):
    work = ["planet", "europe", "germany", "bw", "cal"]

#search bases to use
searchKvStoreOpts = parsedArgs.sb
if (searchKvStoreOpts is None):
    searchKvStoreOpts = ["everything", "bykvall", "bykv"]

#db types to create
if (parsedArgs.dt is None):
    parsedArgs.dt = ["everything", "bykvall", "bykv"]
storeOpts = {}
if ("everything" in parsedArgs.dt):
    storeOpts["everything"] ="se"
if ("bykvall" in parsedArgs.dt):
    storeOpts["bykvall"] = "sik=" + configDir + "/srclists/k_db_reduced.txt,sikv=" + configDir + "/srclists/kv_poi.txt,st"
if ("bykv" in parsedArgs.dt):
    tmp = "sik=" + configDir + "/srclists/k_db_reduced.txt,sikv=" + configDir + "/srclists/kv_poi.txt,"
    tmp += "sk=" + configDir + "/srclists/k_db_reduced.txt,skv=" + configDir + "/srclists/kv_poi.txt"
    
    storeOpts["bykv"] = tmp

#db boundary info
gt2Opts = {}
if (parsedArgs.pa is None):
    parsedArgs.pa = ["pa", "nopa"]
if ("pa" in parsedArgs.pa):
    gt2Opts["pa"] = "pa"
if ("nopa" in parsedArgs.pa):
    gt2Opts["nopa"] = ""

searchTagOpts = {}
if (parsedArgs.tt is None):
    parsedArgs.tt = ["none", "reduced", "all"]
if ("none" in parsedArgs.tt):
    searchTagOpts["none"] = ""
if ("reduced" in parsedArgs.tt):
    searchTagOpts["reduced"] = "tp=" + configDir + "/srclists/k_poi.txt"
if ("all" in parsedArgs.tt):
    searchTagOpts["all"] = "tp=all"

searchNameOpts = {}
if (parsedArgs.sn is None):
    parsedArgs.sn = ["prefix", "suffix"]
if ("prefix" in parsedArgs.sn):
    searchNameOpts["prefix"] = False
if ("suffix" in parsedArgs.sn):
    searchNameOpts["suffix"] = True


#creation opts
defOpts = "-it rlede -tempdir tempdir/ -fasttempdir /dev/shm/osmc"
typeOpts = { "kv": "kvi=" + configDir + "/db/keysValuesToInflate.txt" +
                    ",kdr=" + configDir + "/srclists/k_region_inherit_tags.txt" +
                    ",kvdr=" + configDir + "/srclists/kv_region_inherit_tags.txt"
            }

#all search opts
defSearchOpts = "a,d,mmt=shm"
searchName = {"geocell" : "geocell", "items" : "items", "geoh_gh" : "geoh", "geoh_items" : "items", "geohitems" : "geohitems"}
searchTree = {
                "geocell" : "flattrie",
                "items" : "fitrie",
                "geoh_gh" : "fitrie",
                "geoh_items" : "fitrie",
                "geohitems" : "fitrie"
}
searchKeys = {
                "geocell" : "srclists/k_geocell.txt",
                "items" : "srclists/k_itemsearch.txt",
                "geoh_gh" : "srclists/k_geocell.txt",
                "geoh_items" : "srclists/k_geocell.txt",
                "geohitems" : "srclists/k_geocell.txt"
}

#run info
totalProgramCalls = 0

def getSearchOpts(searchType, nameSuffix, tagOpt):
    ret = "--create-textsearch " + searchName[searchType] + " "
    if nameSuffix:
        ret += "s,tc=1,"
    ret += defSearchOpts + ",t=" + searchTree[searchType] + ",kf=" + configDir + "/" + searchKeys[searchType]
    if (len(tagOpt)):
        ret += "," + tagOpt
    return ret

def callWrapper(cmd, outdir):
    global totalProgramCalls
    totalProgramCalls += 1
    if not dryRun:
        if not os.path.exists(outdir):
            os.makedirs(outdir)
        outFile = open(outdir + "/log", "w")
        print "Created directory " + outdir
    print "Calling: " + cmd
    if not dryRun:
        retcode = subprocess.call(cmd, shell=True, stderr=subprocess.STDOUT, stdout=outFile)
        if (retcode != 0):
            print("Cleaning up due to error")
            subprocess.call("rm /dev/shm/sserialize* /dev/shm/osmc*", shell=True)
        return retcode
    return 0

def getOutDir(name, paType, storeOptType):
	return outDirPath + "/" + name + "_" + paType + "_" + storeOptType

def getSearchOutDir(name, storeOpt, searchType, searchNameType, searchTagType):
    return outDirPath + "/" + name + "_" + storeOpt + "_" + searchType + "_" + searchNameType + "_" + searchTagType + "Tags"

def aop(apop):
    if (len(apop)):
        return "," + apop
    else:
        return apop

failedProgCalls = []
for name in work:
    if createKvs:
        print "Creating the stores"
        gtOpt = gtOpts[name]
        for paType, paOpt in gt2Opts.items():
            for storeOptType, storeOpt in storeOpts.items():
                outdir = getOutDir(name, paType, storeOptType)
                srcFile = srcFilesDir + "/" + srcFiles[name]
                cmdopts = defOpts + " --create-kv " + typeOpts["kv"] + aop(gtOpt) + aop(paOpt) + aop(storeOpt)
                totalcmdopts = cmdopts + " -o " + outdir + " " + srcFile
                timecmdopts = "-vv -o " + outdir + "/time.txt"
                totalcmd = "/usr/bin/time " + timecmdopts + " " + oscarCmd + " " + totalcmdopts
                callWrapper("/usr/bin/time " + timecmdopts + " ./osmfind-create " + totalcmdopts, outdir)
    if not parsedArgs.cs:
        continue
    print "Creating the searches"
    #create the searches
    for searchKvStoreOpt in searchKvStoreOpts:
        for searchType in workSearchTypes:
            srcDir = ""
            if (searchType == "items"):
                srcDir = getOutDir(name, "pa", searchKvStoreOpt)
            else:
                srcDir = srcDir = getOutDir(name, "nopa", searchKvStoreOpt)
            srcKv = srcDir + "/kvstore"
            srcIdx = srcDir + "/index"
            for searchNameType, searchNameOpt in searchNameOpts.items():
                for searchTagType, searchTagOpt in searchTagOpts.items():
                    ctcOpts = ""
                    if (searchType != "geoh"):
                        ctcOpts = getSearchOpts(searchType, searchNameOpt, searchTagOpt)
                    else:
                        ctcOpts = getSearchOpts("geoh_gh", searchNameOpt, "") + " " + getSearchOpts("geoh_items", searchNameOpt, searchTagOpt)
                    cmdopts = defOpts + " " + ctcOpts
                    outdir = getSearchOutDir(name, searchKvStoreOpt, searchType, searchNameType, searchTagType)
                    createcmd = oscarCmd + " " + cmdopts + " -i " + srcIdx + " -o " + outdir + " " + srcKv
                    timecmd = "/usr/bin/time -vv -o " + outdir + "/time.txt "
                    retcode = callWrapper(timecmd + createcmd, outdir)
                    if (retcode != 0):
                        failedProgCalls.append(createcmd)

print "Total number of program calls: " + str(totalProgramCalls)
if (len(failedProgCalls)):
    print("The following calls did not succeed:")
    for cmd in failedProgCalls:
        print(cmd)