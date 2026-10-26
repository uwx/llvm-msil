; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc19.52.37025"

%struct.lldiv_t = type { i64, i64 }
%struct.out = type { ptr, i64, i64 }

$"??_C@_01EEMJAFIK@?6?$AA@" = comdat any

$"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@" = comdat any

$"??_C@_06OJHGLDPL@?$CInull?$CJ?$AA@" = comdat any

$"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@" = comdat any

$"??_C@_03OJHLLCGB@inf?$AA@" = comdat any

$"??_C@_03OEIAHPBN@INF?$AA@" = comdat any

$"??_C@_03LHCJEHON@nan?$AA@" = comdat any

$"??_C@_03LKNCIKJB@NAN?$AA@" = comdat any

$"??_C@_01LFCBOECM@?4?$AA@" = comdat any

@strtok.p = internal global ptr null, align 8
@rng_state = internal unnamed_addr global i64 1, align 8
@"??_C@_01EEMJAFIK@?6?$AA@" = linkonce_odr dso_local unnamed_addr constant [2 x i8] c"\0A\00", comdat, align 1
@"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@" = linkonce_odr dso_local unnamed_addr constant [10 x i8] c"-+   0X0x\00", comdat, align 1
@"??_C@_06OJHGLDPL@?$CInull?$CJ?$AA@" = linkonce_odr dso_local unnamed_addr constant [7 x i8] c"(null)\00", comdat, align 1
@states = internal unnamed_addr constant <{ [58 x i8], [58 x i8], [58 x i8], [58 x i8], [58 x i8], <{ [46 x i8], [12 x i8] }>, [58 x i8], [58 x i8] }> <{ [58 x i8] c"\19\00\0B\00\19\19\19\00\00\00\00\05\00\00\00\00\00\00\09\00\00\00\00\0B\00\00\00\00\00\00\00\00\19\00\0A\0A\19\19\19\03\0A\07\00\01\00\09\0B\18\00\00\09\06\0B\00\00\0B\00\06", [58 x i8] c"\19\00\00\00\19\19\19\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\0E\00\00\00\00\00\00\00\00\19\00\0B\0D\19\19\19\00\0D\00\00\02\00\09\0E\00\00\00\09\00\0E\00\00\0E\00\00", [58 x i8] c"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\0C\00\00\00\00\00\00\00\00\00\00\00\13\00\00\00\00\13\00\00\00\00\09\0C\00\00\00\00\00\0C\00\00\0C\00\00", [58 x i8] c"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\10\00\00\00\00\00\00\00\00\00\00\00\0F\00\00\00\04\0F\00\00\00\00\09\10\00\00\00\00\00\10\00\00\10\00\00", [58 x i8] c"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\12\00\00\00\00\00\00\00\00\00\00\00\11\00\00\00\00\11\00\00\00\00\09\12\00\00\00\00\00\12\00\00\12\00\00", <{ [46 x i8], [12 x i8] }> <{ [46 x i8] c"\1A\00\00\00\1A\1A\1A\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\1A\00\00\00\1A\1A\1A\00\00\00\00\00\00\09", [12 x i8] zeroinitializer }>, [58 x i8] c"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\14\00\00\00\00\00\00\00\00\00\00\00\17\00\00\00\00\17\00\00\00\00\09\14\00\00\00\00\00\14\00\00\14\00\00", [58 x i8] c"\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\16\00\00\00\00\00\00\00\00\00\00\00\15\00\00\00\00\15\00\00\00\00\09\16\00\00\00\00\00\16\00\00\16\00\00" }>, align 16
@xdigits = internal unnamed_addr constant [16 x i8] c"0123456789ABCDEF", align 16
@"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@" = linkonce_odr dso_local unnamed_addr constant [19 x i8] c"-0X+0X 0X-0x+0x 0x\00", comdat, align 1
@"??_C@_03OJHLLCGB@inf?$AA@" = linkonce_odr dso_local unnamed_addr constant [4 x i8] c"inf\00", comdat, align 1
@"??_C@_03OEIAHPBN@INF?$AA@" = linkonce_odr dso_local unnamed_addr constant [4 x i8] c"INF\00", comdat, align 1
@"??_C@_03LHCJEHON@nan?$AA@" = linkonce_odr dso_local unnamed_addr constant [4 x i8] c"nan\00", comdat, align 1
@"??_C@_03LKNCIKJB@NAN?$AA@" = linkonce_odr dso_local unnamed_addr constant [4 x i8] c"NAN\00", comdat, align 1
@"??_C@_01LFCBOECM@?4?$AA@" = linkonce_odr dso_local unnamed_addr constant [2 x i8] c".\00", comdat, align 1

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isalpha(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %or = or i32 %c, 32
  %sub = add i32 %or, -97
  %cmp = icmp ult i32 %sub, 26
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isdigit(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %sub = add i32 %c, -48
  %cmp = icmp ult i32 %sub, 10
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isalnum(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %or.i = or i32 %c, 32
  %0 = add i32 %or.i, -97
  %cmp.i = icmp ult i32 %0, 26
  %sub.i4 = add i32 %c, -48
  %cmp.i5 = icmp ult i32 %sub.i4, 10
  %narrow = or i1 %cmp.i5, %cmp.i
  %lor.ext = zext i1 %narrow to i32
  ret i32 %lor.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isspace(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %cmp = icmp eq i32 %c, 32
  %sub = add i32 %c, -9
  %cmp1 = icmp ult i32 %sub, 5
  %0 = or i1 %cmp, %cmp1
  %lor.ext = zext i1 %0 to i32
  ret i32 %lor.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isblank(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %cmp = icmp eq i32 %c, 32
  %cmp1 = icmp eq i32 %c, 9
  %0 = or i1 %cmp, %cmp1
  %lor.ext = zext i1 %0 to i32
  ret i32 %lor.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @iscntrl(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %cmp = icmp ult i32 %c, 32
  %cmp1 = icmp eq i32 %c, 127
  %0 = or i1 %cmp, %cmp1
  %lor.ext = zext i1 %0 to i32
  ret i32 %lor.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isprint(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %sub = add i32 %c, -32
  %cmp = icmp ult i32 %sub, 95
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isgraph(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %sub = add i32 %c, -33
  %cmp = icmp ult i32 %sub, 94
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local i32 @ispunct(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %0 = add i32 %c, -127
  %cmp.i = icmp ult i32 %0, -94
  br i1 %cmp.i, label %land.end, label %land.rhs

land.rhs:                                         ; preds = %entry
  %or.i.i = or i32 %c, 32
  %1 = add nsw i32 %or.i.i, -123
  %cmp.i.i = icmp ult i32 %1, -26
  %2 = add nsw i32 %c, -58
  %cmp.i5.i = icmp ult i32 %2, -10
  %narrow.i.not = and i1 %cmp.i5.i, %cmp.i.i
  %3 = zext i1 %narrow.i.not to i32
  br label %land.end

land.end:                                         ; preds = %land.rhs, %entry
  %land.ext = phi i32 [ 0, %entry ], [ %3, %land.rhs ]
  ret i32 %land.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isupper(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %sub = add i32 %c, -65
  %cmp = icmp ult i32 %sub, 26
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @islower(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %sub = add i32 %c, -97
  %cmp = icmp ult i32 %sub, 26
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isxdigit(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %sub.i = add i32 %c, -48
  %cmp.i = icmp ult i32 %sub.i, 10
  %or = or i32 %c, 32
  %sub = add i32 %or, -97
  %cmp = icmp ult i32 %sub, 6
  %0 = or i1 %cmp.i, %cmp
  %lor.ext = zext i1 %0 to i32
  ret i32 %lor.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @isascii(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %tobool.not = icmp ult i32 %c, 128
  %lnot.ext = zext i1 %tobool.not to i32
  ret i32 %lnot.ext
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @toascii(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %and = and i32 %c, 127
  ret i32 %and
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @tolower(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %0 = add i32 %c, -91
  %cmp.i = icmp ult i32 %0, -26
  %or = or i32 %c, 32
  %retval.0 = select i1 %cmp.i, i32 %c, i32 %or
  ret i32 %retval.0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @toupper(i32 noundef %c) local_unnamed_addr #0 {
entry:
  %0 = add i32 %c, -123
  %cmp.i = icmp ult i32 %0, -26
  %and = and i32 %c, -33
  %retval.0 = select i1 %cmp.i, i32 %c, i32 %and
  ret i32 %retval.0
}

; Function Attrs: nofree norecurse nosync nounwind memory(read, inaccessiblemem: none)
define dso_local noundef i64 @strnlen(ptr noundef %s, i64 noundef %n) local_unnamed_addr #1 {
entry:
  %tobool.not6 = icmp eq i64 %n, 0
  br i1 %tobool.not6, label %while.end, label %land.rhs.preheader

land.rhs.preheader:                               ; preds = %entry
  %scevgep = getelementptr i8, ptr %s, i64 %n
  br label %land.rhs

land.rhs:                                         ; preds = %while.body, %land.rhs.preheader
  %p.08 = phi ptr [ %incdec.ptr, %while.body ], [ %s, %land.rhs.preheader ]
  %n.addr.07 = phi i64 [ %dec, %while.body ], [ %n, %land.rhs.preheader ]
  %0 = load i8, ptr %p.08, align 1, !tbaa !4
  %tobool1.not = icmp eq i8 %0, 0
  br i1 %tobool1.not, label %while.end, label %while.body

while.body:                                       ; preds = %land.rhs
  %incdec.ptr = getelementptr inbounds i8, ptr %p.08, i64 1
  %dec = add i64 %n.addr.07, -1
  %tobool.not = icmp eq i64 %dec, 0
  br i1 %tobool.not, label %while.end, label %land.rhs, !llvm.loop !7

while.end:                                        ; preds = %while.body, %land.rhs, %entry
  %p.0.lcssa = phi ptr [ %s, %entry ], [ %scevgep, %while.body ], [ %p.08, %land.rhs ]
  %sub.ptr.lhs.cast = ptrtoint ptr %p.0.lcssa to i64
  %sub.ptr.rhs.cast = ptrtoint ptr %s to i64
  %sub.ptr.sub = sub i64 %sub.ptr.lhs.cast, %sub.ptr.rhs.cast
  ret i64 %sub.ptr.sub
}

; Function Attrs: nounwind
define dso_local noundef ptr @strncat(ptr noundef returned %dst, ptr nocapture noundef readonly %src, i64 noundef %n) local_unnamed_addr #2 {
entry:
  %call = tail call i64 @strlen(ptr noundef %dst) #12
  %add.ptr = getelementptr inbounds i8, ptr %dst, i64 %call
  %tobool.not8 = icmp eq i64 %n, 0
  br i1 %tobool.not8, label %while.end, label %land.rhs

land.rhs:                                         ; preds = %while.body, %entry
  %d.011 = phi ptr [ %incdec.ptr2, %while.body ], [ %add.ptr, %entry ]
  %n.addr.010 = phi i64 [ %dec, %while.body ], [ %n, %entry ]
  %src.addr.09 = phi ptr [ %incdec.ptr, %while.body ], [ %src, %entry ]
  %0 = load i8, ptr %src.addr.09, align 1, !tbaa !4
  %tobool1.not = icmp eq i8 %0, 0
  br i1 %tobool1.not, label %while.end, label %while.body

while.body:                                       ; preds = %land.rhs
  %incdec.ptr = getelementptr inbounds i8, ptr %src.addr.09, i64 1
  %incdec.ptr2 = getelementptr inbounds i8, ptr %d.011, i64 1
  store i8 %0, ptr %d.011, align 1, !tbaa !4
  %dec = add i64 %n.addr.010, -1
  %tobool.not = icmp eq i64 %dec, 0
  br i1 %tobool.not, label %while.end, label %land.rhs, !llvm.loop !9

while.end:                                        ; preds = %while.body, %land.rhs, %entry
  %d.0.lcssa = phi ptr [ %add.ptr, %entry ], [ %incdec.ptr2, %while.body ], [ %d.011, %land.rhs ]
  store i8 0, ptr %d.0.lcssa, align 1, !tbaa !4
  ret ptr %dst
}

declare dso_local i64 @strlen(ptr noundef) local_unnamed_addr #3

; Function Attrs: nofree norecurse nosync nounwind memory(read, inaccessiblemem: none)
define dso_local noundef i64 @strspn(ptr noundef %s, ptr nocapture noundef readonly %accept) local_unnamed_addr #1 {
entry:
  %byteset = alloca [4 x i64], align 16
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %byteset) #13
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(32) %byteset, i8 0, i64 32, i1 false)
  %0 = load i8, ptr %accept, align 1, !tbaa !4
  %tobool.not = icmp eq i8 %0, 0
  br i1 %tobool.not, label %cleanup, label %if.end

if.end:                                           ; preds = %entry
  %arrayidx1 = getelementptr inbounds i8, ptr %accept, i64 1
  %1 = load i8, ptr %arrayidx1, align 1, !tbaa !4
  %tobool2.not = icmp eq i8 %1, 0
  br i1 %tobool2.not, label %for.cond, label %for.body9

for.cond:                                         ; preds = %for.cond, %if.end
  %s.addr.0 = phi ptr [ %incdec.ptr, %for.cond ], [ %s, %if.end ]
  %2 = load i8, ptr %s.addr.0, align 1, !tbaa !4
  %cmp = icmp eq i8 %2, %0
  %incdec.ptr = getelementptr inbounds i8, ptr %s.addr.0, i64 1
  br i1 %cmp, label %for.cond, label %for.end, !llvm.loop !10

for.end:                                          ; preds = %for.cond
  %sub.ptr.lhs.cast = ptrtoint ptr %s.addr.0 to i64
  %sub.ptr.rhs.cast = ptrtoint ptr %s to i64
  %sub.ptr.sub = sub i64 %sub.ptr.lhs.cast, %sub.ptr.rhs.cast
  br label %cleanup

for.cond16.preheader:                             ; preds = %for.body9
  %3 = load i8, ptr %s, align 1, !tbaa !4
  %tobool17.not56 = icmp eq i8 %3, 0
  br i1 %tobool17.not56, label %for.end33, label %for.body18

for.body9:                                        ; preds = %for.body9, %if.end
  %4 = phi i8 [ %6, %for.body9 ], [ %0, %if.end ]
  %accept.addr.055 = phi ptr [ %incdec.ptr14, %for.body9 ], [ %accept, %if.end ]
  %conv10 = zext i8 %4 to i32
  %and = and i32 %conv10, 63
  %sh_prom = zext nneg i32 %and to i64
  %shl = shl nuw i64 1, %sh_prom
  %shr = lshr i32 %conv10, 6
  %idxprom = zext nneg i32 %shr to i64
  %arrayidx12 = getelementptr inbounds [4 x i64], ptr %byteset, i64 0, i64 %idxprom
  %5 = load i64, ptr %arrayidx12, align 8, !tbaa !11
  %or = or i64 %5, %shl
  store i64 %or, ptr %arrayidx12, align 8, !tbaa !11
  %incdec.ptr14 = getelementptr inbounds i8, ptr %accept.addr.055, i64 1
  %6 = load i8, ptr %incdec.ptr14, align 1, !tbaa !4
  %tobool8.not = icmp eq i8 %6, 0
  br i1 %tobool8.not, label %for.cond16.preheader, label %for.body9, !llvm.loop !13

for.body18:                                       ; preds = %for.inc31, %for.cond16.preheader
  %7 = phi i8 [ %9, %for.inc31 ], [ %3, %for.cond16.preheader ]
  %s.addr.157 = phi ptr [ %incdec.ptr32, %for.inc31 ], [ %s, %for.cond16.preheader ]
  %conv19 = zext i8 %7 to i32
  %shr20 = lshr i32 %conv19, 6
  %idxprom21 = zext nneg i32 %shr20 to i64
  %arrayidx22 = getelementptr inbounds [4 x i64], ptr %byteset, i64 0, i64 %idxprom21
  %8 = load i64, ptr %arrayidx22, align 8, !tbaa !11
  %and24 = and i32 %conv19, 63
  %sh_prom25 = zext nneg i32 %and24 to i64
  %shl26 = shl nuw i64 1, %sh_prom25
  %and27 = and i64 %8, %shl26
  %tobool28.not = icmp eq i64 %and27, 0
  br i1 %tobool28.not, label %for.end33, label %for.inc31

for.inc31:                                        ; preds = %for.body18
  %incdec.ptr32 = getelementptr inbounds i8, ptr %s.addr.157, i64 1
  %9 = load i8, ptr %incdec.ptr32, align 1, !tbaa !4
  %tobool17.not = icmp eq i8 %9, 0
  br i1 %tobool17.not, label %for.end33, label %for.body18, !llvm.loop !14

for.end33:                                        ; preds = %for.inc31, %for.body18, %for.cond16.preheader
  %s.addr.1.lcssa = phi ptr [ %s, %for.cond16.preheader ], [ %s.addr.157, %for.body18 ], [ %incdec.ptr32, %for.inc31 ]
  %sub.ptr.lhs.cast34 = ptrtoint ptr %s.addr.1.lcssa to i64
  %sub.ptr.rhs.cast35 = ptrtoint ptr %s to i64
  %sub.ptr.sub36 = sub i64 %sub.ptr.lhs.cast34, %sub.ptr.rhs.cast35
  br label %cleanup

cleanup:                                          ; preds = %for.end33, %for.end, %entry
  %retval.0 = phi i64 [ %sub.ptr.sub36, %for.end33 ], [ %sub.ptr.sub, %for.end ], [ 0, %entry ]
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %byteset) #13
  ret i64 %retval.0
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #4

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #5

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #4

; Function Attrs: nounwind
define dso_local i64 @strcspn(ptr noundef %s, ptr nocapture noundef readonly %reject) local_unnamed_addr #2 {
entry:
  %byteset = alloca [4 x i64], align 16
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %byteset) #13
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(32) %byteset, i8 0, i64 32, i1 false)
  %0 = load i8, ptr %reject, align 1, !tbaa !4
  %tobool.not = icmp eq i8 %0, 0
  br i1 %tobool.not, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %call = tail call i64 @strlen(ptr noundef %s) #12
  br label %cleanup

if.end:                                           ; preds = %entry
  %arrayidx1 = getelementptr inbounds i8, ptr %reject, i64 1
  %1 = load i8, ptr %arrayidx1, align 1, !tbaa !4
  %tobool2.not = icmp eq i8 %1, 0
  br i1 %tobool2.not, label %for.cond, label %for.body11

for.cond:                                         ; preds = %for.cond, %if.end
  %s.addr.0 = phi ptr [ %incdec.ptr, %for.cond ], [ %s, %if.end ]
  %2 = load i8, ptr %s.addr.0, align 1, !tbaa !4
  %tobool4.not = icmp eq i8 %2, 0
  %cmp.not = icmp eq i8 %2, %0
  %or.cond = or i1 %tobool4.not, %cmp.not
  %incdec.ptr = getelementptr inbounds i8, ptr %s.addr.0, i64 1
  br i1 %or.cond, label %for.end, label %for.cond, !llvm.loop !15

for.end:                                          ; preds = %for.cond
  %sub.ptr.lhs.cast = ptrtoint ptr %s.addr.0 to i64
  %sub.ptr.rhs.cast = ptrtoint ptr %s to i64
  %sub.ptr.sub = sub i64 %sub.ptr.lhs.cast, %sub.ptr.rhs.cast
  br label %cleanup

for.cond18.preheader:                             ; preds = %for.body11
  %3 = load i8, ptr %s, align 1, !tbaa !4
  %tobool19.not60 = icmp eq i8 %3, 0
  br i1 %tobool19.not60, label %for.end35, label %for.body20

for.body11:                                       ; preds = %for.body11, %if.end
  %4 = phi i8 [ %6, %for.body11 ], [ %0, %if.end ]
  %reject.addr.059 = phi ptr [ %incdec.ptr16, %for.body11 ], [ %reject, %if.end ]
  %conv12 = zext i8 %4 to i32
  %and = and i32 %conv12, 63
  %sh_prom = zext nneg i32 %and to i64
  %shl = shl nuw i64 1, %sh_prom
  %shr = lshr i32 %conv12, 6
  %idxprom = zext nneg i32 %shr to i64
  %arrayidx14 = getelementptr inbounds [4 x i64], ptr %byteset, i64 0, i64 %idxprom
  %5 = load i64, ptr %arrayidx14, align 8, !tbaa !11
  %or = or i64 %5, %shl
  store i64 %or, ptr %arrayidx14, align 8, !tbaa !11
  %incdec.ptr16 = getelementptr inbounds i8, ptr %reject.addr.059, i64 1
  %6 = load i8, ptr %incdec.ptr16, align 1, !tbaa !4
  %tobool10.not = icmp eq i8 %6, 0
  br i1 %tobool10.not, label %for.cond18.preheader, label %for.body11, !llvm.loop !16

for.body20:                                       ; preds = %for.inc33, %for.cond18.preheader
  %7 = phi i8 [ %9, %for.inc33 ], [ %3, %for.cond18.preheader ]
  %s.addr.161 = phi ptr [ %incdec.ptr34, %for.inc33 ], [ %s, %for.cond18.preheader ]
  %conv21 = zext i8 %7 to i32
  %shr22 = lshr i32 %conv21, 6
  %idxprom23 = zext nneg i32 %shr22 to i64
  %arrayidx24 = getelementptr inbounds [4 x i64], ptr %byteset, i64 0, i64 %idxprom23
  %8 = load i64, ptr %arrayidx24, align 8, !tbaa !11
  %and26 = and i32 %conv21, 63
  %sh_prom27 = zext nneg i32 %and26 to i64
  %shl28 = shl nuw i64 1, %sh_prom27
  %and29 = and i64 %8, %shl28
  %tobool30.not = icmp eq i64 %and29, 0
  br i1 %tobool30.not, label %for.inc33, label %for.end35

for.inc33:                                        ; preds = %for.body20
  %incdec.ptr34 = getelementptr inbounds i8, ptr %s.addr.161, i64 1
  %9 = load i8, ptr %incdec.ptr34, align 1, !tbaa !4
  %tobool19.not = icmp eq i8 %9, 0
  br i1 %tobool19.not, label %for.end35, label %for.body20, !llvm.loop !17

for.end35:                                        ; preds = %for.inc33, %for.body20, %for.cond18.preheader
  %s.addr.1.lcssa = phi ptr [ %s, %for.cond18.preheader ], [ %s.addr.161, %for.body20 ], [ %incdec.ptr34, %for.inc33 ]
  %sub.ptr.lhs.cast36 = ptrtoint ptr %s.addr.1.lcssa to i64
  %sub.ptr.rhs.cast37 = ptrtoint ptr %s to i64
  %sub.ptr.sub38 = sub i64 %sub.ptr.lhs.cast36, %sub.ptr.rhs.cast37
  br label %cleanup

cleanup:                                          ; preds = %for.end35, %for.end, %if.then
  %retval.0 = phi i64 [ %sub.ptr.sub38, %for.end35 ], [ %sub.ptr.sub, %for.end ], [ %call, %if.then ]
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %byteset) #13
  ret i64 %retval.0
}

; Function Attrs: nounwind
define dso_local ptr @strpbrk(ptr noundef %s, ptr nocapture noundef readonly %accept) local_unnamed_addr #2 {
entry:
  %byteset.i = alloca [4 x i64], align 16
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %byteset.i) #13
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(32) %byteset.i, i8 0, i64 32, i1 false)
  %0 = load i8, ptr %accept, align 1, !tbaa !4
  %tobool.not.i = icmp eq i8 %0, 0
  br i1 %tobool.not.i, label %if.then.i, label %if.end.i

if.then.i:                                        ; preds = %entry
  %call.i = tail call i64 @strlen(ptr noundef %s) #12
  br label %strcspn.exit

if.end.i:                                         ; preds = %entry
  %arrayidx1.i = getelementptr inbounds i8, ptr %accept, i64 1
  %1 = load i8, ptr %arrayidx1.i, align 1, !tbaa !4
  %tobool2.not.i = icmp eq i8 %1, 0
  br i1 %tobool2.not.i, label %for.cond.i, label %for.body11.i

for.cond.i:                                       ; preds = %for.cond.i, %if.end.i
  %s.addr.0.i = phi ptr [ %incdec.ptr.i, %for.cond.i ], [ %s, %if.end.i ]
  %2 = load i8, ptr %s.addr.0.i, align 1, !tbaa !4
  %tobool4.not.i = icmp eq i8 %2, 0
  %cmp.not.i = icmp eq i8 %2, %0
  %or.cond.i = or i1 %tobool4.not.i, %cmp.not.i
  %incdec.ptr.i = getelementptr inbounds i8, ptr %s.addr.0.i, i64 1
  br i1 %or.cond.i, label %for.end.i, label %for.cond.i, !llvm.loop !15

for.end.i:                                        ; preds = %for.cond.i
  %sub.ptr.lhs.cast.i = ptrtoint ptr %s.addr.0.i to i64
  %sub.ptr.rhs.cast.i = ptrtoint ptr %s to i64
  %sub.ptr.sub.i = sub i64 %sub.ptr.lhs.cast.i, %sub.ptr.rhs.cast.i
  br label %strcspn.exit

for.cond18.preheader.i:                           ; preds = %for.body11.i
  %3 = load i8, ptr %s, align 1, !tbaa !4
  %tobool19.not60.i = icmp eq i8 %3, 0
  br i1 %tobool19.not60.i, label %for.end35.i, label %for.body20.i

for.body11.i:                                     ; preds = %for.body11.i, %if.end.i
  %4 = phi i8 [ %6, %for.body11.i ], [ %0, %if.end.i ]
  %reject.addr.059.i = phi ptr [ %incdec.ptr16.i, %for.body11.i ], [ %accept, %if.end.i ]
  %conv12.i = zext i8 %4 to i32
  %and.i = and i32 %conv12.i, 63
  %sh_prom.i = zext nneg i32 %and.i to i64
  %shl.i = shl nuw i64 1, %sh_prom.i
  %shr.i = lshr i32 %conv12.i, 6
  %idxprom.i = zext nneg i32 %shr.i to i64
  %arrayidx14.i = getelementptr inbounds [4 x i64], ptr %byteset.i, i64 0, i64 %idxprom.i
  %5 = load i64, ptr %arrayidx14.i, align 8, !tbaa !11
  %or.i = or i64 %shl.i, %5
  store i64 %or.i, ptr %arrayidx14.i, align 8, !tbaa !11
  %incdec.ptr16.i = getelementptr inbounds i8, ptr %reject.addr.059.i, i64 1
  %6 = load i8, ptr %incdec.ptr16.i, align 1, !tbaa !4
  %tobool10.not.i = icmp eq i8 %6, 0
  br i1 %tobool10.not.i, label %for.cond18.preheader.i, label %for.body11.i, !llvm.loop !16

for.body20.i:                                     ; preds = %for.inc33.i, %for.cond18.preheader.i
  %7 = phi i8 [ %9, %for.inc33.i ], [ %3, %for.cond18.preheader.i ]
  %s.addr.161.i = phi ptr [ %incdec.ptr34.i, %for.inc33.i ], [ %s, %for.cond18.preheader.i ]
  %conv21.i = zext i8 %7 to i32
  %shr22.i = lshr i32 %conv21.i, 6
  %idxprom23.i = zext nneg i32 %shr22.i to i64
  %arrayidx24.i = getelementptr inbounds [4 x i64], ptr %byteset.i, i64 0, i64 %idxprom23.i
  %8 = load i64, ptr %arrayidx24.i, align 8, !tbaa !11
  %and26.i = and i32 %conv21.i, 63
  %sh_prom27.i = zext nneg i32 %and26.i to i64
  %shl28.i = shl nuw i64 1, %sh_prom27.i
  %and29.i = and i64 %shl28.i, %8
  %tobool30.not.i = icmp eq i64 %and29.i, 0
  br i1 %tobool30.not.i, label %for.inc33.i, label %for.end35.i

for.inc33.i:                                      ; preds = %for.body20.i
  %incdec.ptr34.i = getelementptr inbounds i8, ptr %s.addr.161.i, i64 1
  %9 = load i8, ptr %incdec.ptr34.i, align 1, !tbaa !4
  %tobool19.not.i = icmp eq i8 %9, 0
  br i1 %tobool19.not.i, label %for.end35.i, label %for.body20.i, !llvm.loop !17

for.end35.i:                                      ; preds = %for.inc33.i, %for.body20.i, %for.cond18.preheader.i
  %s.addr.1.lcssa.i = phi ptr [ %s, %for.cond18.preheader.i ], [ %incdec.ptr34.i, %for.inc33.i ], [ %s.addr.161.i, %for.body20.i ]
  %sub.ptr.lhs.cast36.i = ptrtoint ptr %s.addr.1.lcssa.i to i64
  %sub.ptr.rhs.cast37.i = ptrtoint ptr %s to i64
  %sub.ptr.sub38.i = sub i64 %sub.ptr.lhs.cast36.i, %sub.ptr.rhs.cast37.i
  br label %strcspn.exit

strcspn.exit:                                     ; preds = %for.end35.i, %for.end.i, %if.then.i
  %retval.0.i = phi i64 [ %sub.ptr.sub38.i, %for.end35.i ], [ %sub.ptr.sub.i, %for.end.i ], [ %call.i, %if.then.i ]
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %byteset.i) #13
  %add.ptr = getelementptr inbounds i8, ptr %s, i64 %retval.0.i
  %10 = load i8, ptr %add.ptr, align 1, !tbaa !4
  %tobool.not = icmp eq i8 %10, 0
  %cond = select i1 %tobool.not, ptr null, ptr %add.ptr
  ret ptr %cond
}

; Function Attrs: nounwind
define dso_local noundef ptr @strtok_r(ptr noundef %s, ptr nocapture noundef readonly %sep, ptr nocapture noundef %p) local_unnamed_addr #2 {
entry:
  %byteset.i22 = alloca [4 x i64], align 16
  %byteset.i = alloca [4 x i64], align 16
  %tobool.not = icmp eq ptr %s, null
  br i1 %tobool.not, label %land.lhs.true, label %if.end

land.lhs.true:                                    ; preds = %entry
  %0 = load ptr, ptr %p, align 8, !tbaa !18
  %tobool1.not = icmp eq ptr %0, null
  br i1 %tobool1.not, label %return, label %if.end

if.end:                                           ; preds = %land.lhs.true, %entry
  %s.addr.0 = phi ptr [ %s, %entry ], [ %0, %land.lhs.true ]
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %byteset.i) #13
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(32) %byteset.i, i8 0, i64 32, i1 false)
  %1 = load i8, ptr %sep, align 1, !tbaa !4
  %tobool.not.i = icmp eq i8 %1, 0
  br i1 %tobool.not.i, label %strspn.exit.thread, label %if.end.i

if.end.i:                                         ; preds = %if.end
  %arrayidx1.i = getelementptr inbounds i8, ptr %sep, i64 1
  %2 = load i8, ptr %arrayidx1.i, align 1, !tbaa !4
  %tobool2.not.i = icmp eq i8 %2, 0
  br i1 %tobool2.not.i, label %for.cond.i, label %for.body9.i

for.cond.i:                                       ; preds = %for.cond.i, %if.end.i
  %s.addr.0.i = phi ptr [ %incdec.ptr.i, %for.cond.i ], [ %s.addr.0, %if.end.i ]
  %3 = load i8, ptr %s.addr.0.i, align 1, !tbaa !4
  %cmp.i = icmp eq i8 %3, %1
  %incdec.ptr.i = getelementptr inbounds i8, ptr %s.addr.0.i, i64 1
  br i1 %cmp.i, label %for.cond.i, label %strspn.exit, !llvm.loop !10

for.cond16.preheader.i:                           ; preds = %for.body9.i
  %4 = load i8, ptr %s.addr.0, align 1, !tbaa !4
  %tobool17.not56.i = icmp eq i8 %4, 0
  br i1 %tobool17.not56.i, label %strspn.exit, label %for.body18.i

for.body9.i:                                      ; preds = %for.body9.i, %if.end.i
  %5 = phi i8 [ %7, %for.body9.i ], [ %1, %if.end.i ]
  %accept.addr.055.i = phi ptr [ %incdec.ptr14.i, %for.body9.i ], [ %sep, %if.end.i ]
  %conv10.i = zext i8 %5 to i32
  %and.i = and i32 %conv10.i, 63
  %sh_prom.i = zext nneg i32 %and.i to i64
  %shl.i = shl nuw i64 1, %sh_prom.i
  %shr.i = lshr i32 %conv10.i, 6
  %idxprom.i = zext nneg i32 %shr.i to i64
  %arrayidx12.i = getelementptr inbounds [4 x i64], ptr %byteset.i, i64 0, i64 %idxprom.i
  %6 = load i64, ptr %arrayidx12.i, align 8, !tbaa !11
  %or.i = or i64 %shl.i, %6
  store i64 %or.i, ptr %arrayidx12.i, align 8, !tbaa !11
  %incdec.ptr14.i = getelementptr inbounds i8, ptr %accept.addr.055.i, i64 1
  %7 = load i8, ptr %incdec.ptr14.i, align 1, !tbaa !4
  %tobool8.not.i = icmp eq i8 %7, 0
  br i1 %tobool8.not.i, label %for.cond16.preheader.i, label %for.body9.i, !llvm.loop !13

for.body18.i:                                     ; preds = %for.inc31.i, %for.cond16.preheader.i
  %8 = phi i8 [ %10, %for.inc31.i ], [ %4, %for.cond16.preheader.i ]
  %s.addr.157.i = phi ptr [ %incdec.ptr32.i, %for.inc31.i ], [ %s.addr.0, %for.cond16.preheader.i ]
  %conv19.i = zext i8 %8 to i32
  %shr20.i = lshr i32 %conv19.i, 6
  %idxprom21.i = zext nneg i32 %shr20.i to i64
  %arrayidx22.i = getelementptr inbounds [4 x i64], ptr %byteset.i, i64 0, i64 %idxprom21.i
  %9 = load i64, ptr %arrayidx22.i, align 8, !tbaa !11
  %and24.i = and i32 %conv19.i, 63
  %sh_prom25.i = zext nneg i32 %and24.i to i64
  %shl26.i = shl nuw i64 1, %sh_prom25.i
  %and27.i = and i64 %shl26.i, %9
  %tobool28.not.i = icmp eq i64 %and27.i, 0
  br i1 %tobool28.not.i, label %strspn.exit, label %for.inc31.i

for.inc31.i:                                      ; preds = %for.body18.i
  %incdec.ptr32.i = getelementptr inbounds i8, ptr %s.addr.157.i, i64 1
  %10 = load i8, ptr %incdec.ptr32.i, align 1, !tbaa !4
  %tobool17.not.i = icmp eq i8 %10, 0
  br i1 %tobool17.not.i, label %strspn.exit, label %for.body18.i, !llvm.loop !14

strspn.exit:                                      ; preds = %for.inc31.i, %for.body18.i, %for.cond16.preheader.i, %for.cond.i
  %s.addr.0.i.lcssa.sink = phi ptr [ %s.addr.0, %for.cond16.preheader.i ], [ %s.addr.0.i, %for.cond.i ], [ %incdec.ptr32.i, %for.inc31.i ], [ %s.addr.157.i, %for.body18.i ]
  %sub.ptr.lhs.cast.i = ptrtoint ptr %s.addr.0.i.lcssa.sink to i64
  %sub.ptr.rhs.cast.i = ptrtoint ptr %s.addr.0 to i64
  %sub.ptr.sub.i = sub i64 %sub.ptr.lhs.cast.i, %sub.ptr.rhs.cast.i
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %byteset.i) #13
  %add.ptr = getelementptr inbounds i8, ptr %s.addr.0, i64 %sub.ptr.sub.i
  %11 = load i8, ptr %add.ptr, align 1, !tbaa !4
  %tobool2.not = icmp eq i8 %11, 0
  br i1 %tobool2.not, label %if.then3, label %if.end4

strspn.exit.thread:                               ; preds = %if.end
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %byteset.i) #13
  %12 = load i8, ptr %s.addr.0, align 1, !tbaa !4
  %tobool2.not44 = icmp eq i8 %12, 0
  br i1 %tobool2.not44, label %if.then3, label %if.end4.thread

if.end4.thread:                                   ; preds = %strspn.exit.thread
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %byteset.i22) #13
  %call.i = tail call i64 @strlen(ptr noundef nonnull %s.addr.0) #12
  br label %strcspn.exit

if.then3:                                         ; preds = %strspn.exit.thread, %strspn.exit
  store ptr null, ptr %p, align 8, !tbaa !18
  br label %return

if.end4:                                          ; preds = %strspn.exit
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %byteset.i22) #13
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(32) %byteset.i22, i8 0, i64 32, i1 false)
  br i1 %tobool2.not.i, label %for.cond.i35, label %for.body11.i

for.cond.i35:                                     ; preds = %for.cond.i35, %if.end4
  %s.addr.0.i36 = phi ptr [ %incdec.ptr.i37, %for.cond.i35 ], [ %add.ptr, %if.end4 ]
  %13 = load i8, ptr %s.addr.0.i36, align 1, !tbaa !4
  %tobool4.not.i = icmp eq i8 %13, 0
  %cmp.not.i = icmp eq i8 %13, %1
  %or.cond.i = or i1 %tobool4.not.i, %cmp.not.i
  %incdec.ptr.i37 = getelementptr inbounds i8, ptr %s.addr.0.i36, i64 1
  br i1 %or.cond.i, label %for.end.i38, label %for.cond.i35, !llvm.loop !15

for.end.i38:                                      ; preds = %for.cond.i35
  %sub.ptr.lhs.cast.i39 = ptrtoint ptr %s.addr.0.i36 to i64
  %sub.ptr.rhs.cast.i40 = ptrtoint ptr %add.ptr to i64
  %sub.ptr.sub.i41 = sub i64 %sub.ptr.lhs.cast.i39, %sub.ptr.rhs.cast.i40
  br label %strcspn.exit

for.body11.i:                                     ; preds = %for.body11.i, %if.end4
  %14 = phi i8 [ %16, %for.body11.i ], [ %1, %if.end4 ]
  %reject.addr.059.i = phi ptr [ %incdec.ptr16.i, %for.body11.i ], [ %sep, %if.end4 ]
  %conv12.i = zext i8 %14 to i32
  %and.i27 = and i32 %conv12.i, 63
  %sh_prom.i28 = zext nneg i32 %and.i27 to i64
  %shl.i29 = shl nuw i64 1, %sh_prom.i28
  %shr.i30 = lshr i32 %conv12.i, 6
  %idxprom.i31 = zext nneg i32 %shr.i30 to i64
  %arrayidx14.i = getelementptr inbounds [4 x i64], ptr %byteset.i22, i64 0, i64 %idxprom.i31
  %15 = load i64, ptr %arrayidx14.i, align 8, !tbaa !11
  %or.i32 = or i64 %shl.i29, %15
  store i64 %or.i32, ptr %arrayidx14.i, align 8, !tbaa !11
  %incdec.ptr16.i = getelementptr inbounds i8, ptr %reject.addr.059.i, i64 1
  %16 = load i8, ptr %incdec.ptr16.i, align 1, !tbaa !4
  %tobool10.not.i = icmp eq i8 %16, 0
  br i1 %tobool10.not.i, label %for.body20.i, label %for.body11.i, !llvm.loop !16

for.body20.i:                                     ; preds = %for.inc33.i, %for.body11.i
  %17 = phi i8 [ %19, %for.inc33.i ], [ %11, %for.body11.i ]
  %s.addr.161.i = phi ptr [ %incdec.ptr34.i, %for.inc33.i ], [ %add.ptr, %for.body11.i ]
  %conv21.i = zext i8 %17 to i32
  %shr22.i = lshr i32 %conv21.i, 6
  %idxprom23.i = zext nneg i32 %shr22.i to i64
  %arrayidx24.i = getelementptr inbounds [4 x i64], ptr %byteset.i22, i64 0, i64 %idxprom23.i
  %18 = load i64, ptr %arrayidx24.i, align 8, !tbaa !11
  %and26.i = and i32 %conv21.i, 63
  %sh_prom27.i = zext nneg i32 %and26.i to i64
  %shl28.i = shl nuw i64 1, %sh_prom27.i
  %and29.i = and i64 %shl28.i, %18
  %tobool30.not.i = icmp eq i64 %and29.i, 0
  br i1 %tobool30.not.i, label %for.inc33.i, label %for.end35.i

for.inc33.i:                                      ; preds = %for.body20.i
  %incdec.ptr34.i = getelementptr inbounds i8, ptr %s.addr.161.i, i64 1
  %19 = load i8, ptr %incdec.ptr34.i, align 1, !tbaa !4
  %tobool19.not.i = icmp eq i8 %19, 0
  br i1 %tobool19.not.i, label %for.end35.i, label %for.body20.i, !llvm.loop !17

for.end35.i:                                      ; preds = %for.inc33.i, %for.body20.i
  %s.addr.1.lcssa.i33.ph = phi ptr [ %s.addr.161.i, %for.body20.i ], [ %incdec.ptr34.i, %for.inc33.i ]
  %sub.ptr.lhs.cast36.i = ptrtoint ptr %s.addr.1.lcssa.i33.ph to i64
  %sub.ptr.rhs.cast37.i = ptrtoint ptr %add.ptr to i64
  %sub.ptr.sub38.i = sub i64 %sub.ptr.lhs.cast36.i, %sub.ptr.rhs.cast37.i
  br label %strcspn.exit

strcspn.exit:                                     ; preds = %for.end35.i, %for.end.i38, %if.end4.thread
  %add.ptr4547 = phi ptr [ %add.ptr, %for.end35.i ], [ %add.ptr, %for.end.i38 ], [ %s.addr.0, %if.end4.thread ]
  %retval.0.i34 = phi i64 [ %sub.ptr.sub38.i, %for.end35.i ], [ %sub.ptr.sub.i41, %for.end.i38 ], [ %call.i, %if.end4.thread ]
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %byteset.i22) #13
  %add.ptr6 = getelementptr inbounds i8, ptr %add.ptr4547, i64 %retval.0.i34
  store ptr %add.ptr6, ptr %p, align 8, !tbaa !18
  %20 = load i8, ptr %add.ptr6, align 1, !tbaa !4
  %tobool7.not = icmp eq i8 %20, 0
  br i1 %tobool7.not, label %if.else, label %if.then8

if.then8:                                         ; preds = %strcspn.exit
  %incdec.ptr = getelementptr inbounds i8, ptr %add.ptr6, i64 1
  store ptr %incdec.ptr, ptr %p, align 8, !tbaa !18
  store i8 0, ptr %add.ptr6, align 1, !tbaa !4
  br label %return

if.else:                                          ; preds = %strcspn.exit
  store ptr null, ptr %p, align 8, !tbaa !18
  br label %return

return:                                           ; preds = %if.else, %if.then8, %if.then3, %land.lhs.true
  %retval.0 = phi ptr [ null, %if.then3 ], [ null, %land.lhs.true ], [ %add.ptr4547, %if.else ], [ %add.ptr4547, %if.then8 ]
  ret ptr %retval.0
}

; Function Attrs: nounwind
define dso_local noundef ptr @strtok(ptr noundef %s, ptr nocapture noundef readonly %sep) local_unnamed_addr #2 {
entry:
  %call = tail call ptr @strtok_r(ptr noundef %s, ptr noundef %sep, ptr noundef nonnull @strtok.p) #14
  ret ptr %call
}

; Function Attrs: nounwind
define dso_local i32 @strcasecmp(ptr nocapture noundef readonly %a, ptr nocapture noundef readonly %b) local_unnamed_addr #2 {
entry:
  %0 = load i8, ptr %a, align 1, !tbaa !4
  %tobool.not25 = icmp eq i8 %0, 0
  br i1 %tobool.not25, label %for.end, label %land.lhs.true

land.lhs.true:                                    ; preds = %for.inc, %entry
  %1 = phi i8 [ %4, %for.inc ], [ %0, %entry ]
  %r.027 = phi ptr [ %incdec.ptr11, %for.inc ], [ %b, %entry ]
  %l.026 = phi ptr [ %incdec.ptr, %for.inc ], [ %a, %entry ]
  %conv28 = zext i8 %1 to i32
  %2 = load i8, ptr %r.027, align 1, !tbaa !4
  %tobool2.not = icmp eq i8 %2, 0
  br i1 %tobool2.not, label %for.end.loopexit, label %land.rhs

land.rhs:                                         ; preds = %land.lhs.true
  %cmp = icmp eq i8 %1, %2
  br i1 %cmp, label %for.inc, label %lor.rhs

lor.rhs:                                          ; preds = %land.rhs
  %call = tail call i32 @tolower(i32 noundef %conv28) #12
  %3 = load i8, ptr %r.027, align 1, !tbaa !4
  %conv7 = zext i8 %3 to i32
  %call8 = tail call i32 @tolower(i32 noundef %conv7) #12
  %cmp9 = icmp eq i32 %call, %call8
  br i1 %cmp9, label %for.inc, label %lor.rhs.for.end.loopexit_crit_edge

lor.rhs.for.end.loopexit_crit_edge:               ; preds = %lor.rhs
  %.pre.pre = load i8, ptr %l.026, align 1, !tbaa !4
  br label %for.end.loopexit

for.inc:                                          ; preds = %lor.rhs, %land.rhs
  %incdec.ptr = getelementptr inbounds i8, ptr %l.026, i64 1
  %incdec.ptr11 = getelementptr inbounds i8, ptr %r.027, i64 1
  %4 = load i8, ptr %incdec.ptr, align 1, !tbaa !4
  %tobool.not = icmp eq i8 %4, 0
  br i1 %tobool.not, label %for.end.loopexit, label %land.lhs.true, !llvm.loop !20

for.end.loopexit:                                 ; preds = %for.inc, %lor.rhs.for.end.loopexit_crit_edge, %land.lhs.true
  %.pre = phi i8 [ %.pre.pre, %lor.rhs.for.end.loopexit_crit_edge ], [ 0, %for.inc ], [ %1, %land.lhs.true ]
  %r.0.lcssa.ph = phi ptr [ %r.027, %lor.rhs.for.end.loopexit_crit_edge ], [ %incdec.ptr11, %for.inc ], [ %r.027, %land.lhs.true ]
  %5 = zext i8 %.pre to i32
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %entry
  %conv12 = phi i32 [ 0, %entry ], [ %5, %for.end.loopexit ]
  %r.0.lcssa = phi ptr [ %b, %entry ], [ %r.0.lcssa.ph, %for.end.loopexit ]
  %call13 = tail call i32 @tolower(i32 noundef %conv12) #12
  %6 = load i8, ptr %r.0.lcssa, align 1, !tbaa !4
  %conv14 = zext i8 %6 to i32
  %call15 = tail call i32 @tolower(i32 noundef %conv14) #12
  %sub = sub nsw i32 %call13, %call15
  ret i32 %sub
}

; Function Attrs: nounwind
define dso_local i32 @strncasecmp(ptr nocapture noundef readonly %a, ptr nocapture noundef readonly %b, i64 noundef %n) local_unnamed_addr #2 {
entry:
  %tobool.not = icmp eq i64 %n, 0
  br i1 %tobool.not, label %cleanup, label %for.cond.preheader

for.cond.preheader:                               ; preds = %entry
  %0 = load i8, ptr %a, align 1, !tbaa !4
  %tobool1.not33 = icmp eq i8 %0, 0
  br i1 %tobool1.not33, label %for.end, label %land.lhs.true

land.lhs.true:                                    ; preds = %for.inc, %for.cond.preheader
  %1 = phi i8 [ %4, %for.inc ], [ %0, %for.cond.preheader ]
  %n.addr.036.in = phi i64 [ %n.addr.036, %for.inc ], [ %n, %for.cond.preheader ]
  %r.035 = phi ptr [ %incdec.ptr14, %for.inc ], [ %b, %for.cond.preheader ]
  %l.034 = phi ptr [ %incdec.ptr, %for.inc ], [ %a, %for.cond.preheader ]
  %n.addr.036 = add i64 %n.addr.036.in, -1
  %conv37 = zext i8 %1 to i32
  %2 = load i8, ptr %r.035, align 1, !tbaa !4
  %tobool3 = icmp ne i8 %2, 0
  %tobool5 = icmp ne i64 %n.addr.036, 0
  %or.cond = select i1 %tobool3, i1 %tobool5, i1 false
  br i1 %or.cond, label %land.rhs, label %for.end.loopexit

land.rhs:                                         ; preds = %land.lhs.true
  %cmp = icmp eq i8 %1, %2
  br i1 %cmp, label %for.inc, label %lor.rhs

lor.rhs:                                          ; preds = %land.rhs
  %call = tail call i32 @tolower(i32 noundef %conv37) #12
  %3 = load i8, ptr %r.035, align 1, !tbaa !4
  %conv10 = zext i8 %3 to i32
  %call11 = tail call i32 @tolower(i32 noundef %conv10) #12
  %cmp12 = icmp eq i32 %call, %call11
  br i1 %cmp12, label %for.inc, label %lor.rhs.for.end.loopexit_crit_edge

lor.rhs.for.end.loopexit_crit_edge:               ; preds = %lor.rhs
  %.pre.pre = load i8, ptr %l.034, align 1, !tbaa !4
  br label %for.end.loopexit

for.inc:                                          ; preds = %lor.rhs, %land.rhs
  %incdec.ptr = getelementptr inbounds i8, ptr %l.034, i64 1
  %incdec.ptr14 = getelementptr inbounds i8, ptr %r.035, i64 1
  %4 = load i8, ptr %incdec.ptr, align 1, !tbaa !4
  %tobool1.not = icmp eq i8 %4, 0
  br i1 %tobool1.not, label %for.end.loopexit, label %land.lhs.true, !llvm.loop !21

for.end.loopexit:                                 ; preds = %for.inc, %lor.rhs.for.end.loopexit_crit_edge, %land.lhs.true
  %.pre = phi i8 [ %.pre.pre, %lor.rhs.for.end.loopexit_crit_edge ], [ 0, %for.inc ], [ %1, %land.lhs.true ]
  %r.0.lcssa.ph = phi ptr [ %r.035, %lor.rhs.for.end.loopexit_crit_edge ], [ %incdec.ptr14, %for.inc ], [ %r.035, %land.lhs.true ]
  %5 = zext i8 %.pre to i32
  br label %for.end

for.end:                                          ; preds = %for.end.loopexit, %for.cond.preheader
  %conv16 = phi i32 [ 0, %for.cond.preheader ], [ %5, %for.end.loopexit ]
  %r.0.lcssa = phi ptr [ %b, %for.cond.preheader ], [ %r.0.lcssa.ph, %for.end.loopexit ]
  %call17 = tail call i32 @tolower(i32 noundef %conv16) #12
  %6 = load i8, ptr %r.0.lcssa, align 1, !tbaa !4
  %conv18 = zext i8 %6 to i32
  %call19 = tail call i32 @tolower(i32 noundef %conv18) #12
  %sub = sub nsw i32 %call17, %call19
  br label %cleanup

cleanup:                                          ; preds = %for.end, %entry
  %retval.0 = phi i32 [ %sub, %for.end ], [ 0, %entry ]
  ret i32 %retval.0
}

; Function Attrs: nofree norecurse nosync nounwind memory(read, inaccessiblemem: none)
define dso_local noundef ptr @memchr(ptr noundef readonly %src, i32 noundef %c, i64 noundef %n) local_unnamed_addr #1 {
entry:
  %tobool.not8 = icmp eq i64 %n, 0
  br i1 %tobool.not8, label %cleanup, label %for.body.lr.ph

for.body.lr.ph:                                   ; preds = %entry
  %0 = trunc i32 %c to i8
  br label %for.body

for.body:                                         ; preds = %for.inc, %for.body.lr.ph
  %s.010 = phi ptr [ %src, %for.body.lr.ph ], [ %incdec.ptr, %for.inc ]
  %n.addr.09 = phi i64 [ %n, %for.body.lr.ph ], [ %dec, %for.inc ]
  %1 = load i8, ptr %s.010, align 1, !tbaa !4
  %cmp = icmp eq i8 %1, %0
  br i1 %cmp, label %cleanup, label %for.inc

for.inc:                                          ; preds = %for.body
  %dec = add i64 %n.addr.09, -1
  %incdec.ptr = getelementptr inbounds i8, ptr %s.010, i64 1
  %tobool.not = icmp eq i64 %dec, 0
  br i1 %tobool.not, label %cleanup, label %for.body, !llvm.loop !22

cleanup:                                          ; preds = %for.inc, %for.body, %entry
  %retval.0 = phi ptr [ null, %entry ], [ null, %for.inc ], [ %s.010, %for.body ]
  ret ptr %retval.0
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define dso_local noundef ptr @memrchr(ptr noundef readonly %src, i32 noundef %c, i64 noundef %n) local_unnamed_addr #6 {
entry:
  %0 = trunc i32 %c to i8
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %n.addr.0 = phi i64 [ %n, %entry ], [ %dec, %while.body ]
  %tobool.not = icmp eq i64 %n.addr.0, 0
  br i1 %tobool.not, label %cleanup, label %while.body

while.body:                                       ; preds = %while.cond
  %dec = add i64 %n.addr.0, -1
  %arrayidx = getelementptr inbounds i8, ptr %src, i64 %dec
  %1 = load i8, ptr %arrayidx, align 1, !tbaa !4
  %cmp = icmp eq i8 %1, %0
  br i1 %cmp, label %cleanup.split.loop.exit10, label %while.cond, !llvm.loop !23

cleanup.split.loop.exit10:                        ; preds = %while.body
  %arrayidx.le = getelementptr inbounds i8, ptr %src, i64 %dec
  br label %cleanup

cleanup:                                          ; preds = %cleanup.split.loop.exit10, %while.cond
  %retval.0 = phi ptr [ %arrayidx.le, %cleanup.split.loop.exit10 ], [ null, %while.cond ]
  ret ptr %retval.0
}

; Function Attrs: nounwind
define dso_local i32 @strcoll(ptr noundef %a, ptr noundef %b) local_unnamed_addr #2 {
entry:
  %call = tail call i32 @strcmp(ptr noundef %a, ptr noundef %b) #12
  ret i32 %call
}

declare dso_local i32 @strcmp(ptr noundef, ptr noundef) local_unnamed_addr #3

; Function Attrs: nounwind
define dso_local i64 @strxfrm(ptr noundef %dst, ptr noundef %src, i64 noundef %n) local_unnamed_addr #2 {
entry:
  %call = tail call i64 @strlen(ptr noundef %src) #12
  %tobool.not = icmp eq i64 %n, 0
  br i1 %tobool.not, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %cmp = icmp ult i64 %call, %n
  %sub = add i64 %n, -1
  %cond = select i1 %cmp, i64 %call, i64 %sub
  %call1 = tail call ptr @memcpy(ptr noundef %dst, ptr noundef %src, i64 noundef %cond) #12
  %arrayidx = getelementptr inbounds i8, ptr %dst, i64 %cond
  store i8 0, ptr %arrayidx, align 1, !tbaa !4
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret i64 %call
}

declare dso_local ptr @memcpy(ptr noundef, ptr noundef, i64 noundef) local_unnamed_addr #3

; Function Attrs: nounwind
define dso_local i64 @strtoll(ptr noundef %s, ptr noundef %endp, i32 noundef %base) local_unnamed_addr #2 {
entry:
  %neg = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg) #13
  %call = call fastcc i64 @scan(ptr noundef %s, ptr noundef %endp, i32 noundef %base, i64 noundef 9223372036854775807, ptr noundef nonnull %neg) #14
  %0 = load i32, ptr %neg, align 4, !tbaa !24
  %tobool.not = icmp eq i32 %0, 0
  %sub = sub nsw i64 0, %call
  %cond = select i1 %tobool.not, i64 %call, i64 %sub
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg) #13
  ret i64 %cond
}

; Function Attrs: nounwind
define internal fastcc i64 @scan(ptr noundef %s, ptr noundef writeonly %endp, i32 noundef %base, i64 noundef %lim, ptr nocapture noundef writeonly %negative) unnamed_addr #2 {
entry:
  %cmp.not = icmp ne i32 %base, 0
  %0 = add i32 %base, -37
  %or.cond = icmp ult i32 %0, -35
  %or.cond166 = and i1 %cmp.not, %or.cond
  br i1 %or.cond166, label %if.then, label %while.cond

if.then:                                          ; preds = %entry
  %tobool.not = icmp eq ptr %endp, null
  br i1 %tobool.not, label %cleanup, label %cleanup.sink.split

while.cond:                                       ; preds = %while.cond, %entry
  %p.0 = phi ptr [ %incdec.ptr, %while.cond ], [ %s, %entry ]
  %1 = load i8, ptr %p.0, align 1, !tbaa !4
  %conv = zext i8 %1 to i32
  %call = tail call i32 @isspace(i32 noundef %conv) #12
  %tobool5.not = icmp eq i32 %call, 0
  %incdec.ptr = getelementptr inbounds i8, ptr %p.0, i64 1
  br i1 %tobool5.not, label %while.end, label %while.cond, !llvm.loop !26

while.end:                                        ; preds = %while.cond
  %2 = load i8, ptr %p.0, align 1, !tbaa !4
  switch i8 %2, label %if.end18 [
    i8 43, label %if.then13
    i8 45, label %if.then13
  ]

if.then13:                                        ; preds = %while.end, %while.end
  %cmp15 = icmp eq i8 %2, 45
  %conv16 = zext i1 %cmp15 to i32
  %.pre = load i8, ptr %incdec.ptr, align 1, !tbaa !4
  br label %if.end18

if.end18:                                         ; preds = %if.then13, %while.end
  %3 = phi i8 [ %.pre, %if.then13 ], [ %2, %while.end ]
  %p.1 = phi ptr [ %incdec.ptr, %if.then13 ], [ %p.0, %while.end ]
  %neg.0 = phi i32 [ %conv16, %if.then13 ], [ 0, %while.end ]
  %cmp20 = icmp eq i32 %base, 0
  %cmp23 = icmp eq i32 %base, 16
  %or.cond108 = or i1 %cmp20, %cmp23
  %cmp26 = icmp eq i8 %3, 48
  %or.cond109 = select i1 %or.cond108, i1 %cmp26, i1 false
  br i1 %or.cond109, label %if.then28, label %if.else50

if.then28:                                        ; preds = %if.end18
  %incdec.ptr29 = getelementptr inbounds i8, ptr %p.1, i64 1
  %4 = load i8, ptr %incdec.ptr29, align 1, !tbaa !4
  %conv30 = zext i8 %4 to i32
  %5 = and i32 %conv30, 223
  %cmp31 = icmp eq i32 %5, 88
  br i1 %cmp31, label %if.then33, label %if.else

if.then33:                                        ; preds = %if.then28
  %incdec.ptr34 = getelementptr inbounds i8, ptr %p.1, i64 2
  %6 = load i8, ptr %incdec.ptr34, align 1, !tbaa !4
  %conv35 = zext i8 %6 to i32
  %7 = add nsw i32 %conv35, -48
  %or.cond.i = icmp ult i32 %7, 10
  br i1 %or.cond.i, label %if.end55, label %if.end.i

if.end.i:                                         ; preds = %if.then33
  %8 = add nsw i32 %conv35, -97
  %or.cond15.i = icmp ult i32 %8, 26
  br i1 %or.cond15.i, label %if.then5.i, label %if.end7.i

if.then5.i:                                       ; preds = %if.end.i
  %add.i = add nsw i32 %conv35, -87
  br label %digitval.exit

if.end7.i:                                        ; preds = %if.end.i
  %9 = add nsw i32 %conv35, -65
  %or.cond16.i = icmp ult i32 %9, 26
  %add13.i = add nsw i32 %conv35, -55
  br i1 %or.cond16.i, label %digitval.exit, label %if.end55

digitval.exit:                                    ; preds = %if.end7.i, %if.then5.i
  %retval.0.i = phi i32 [ %add.i, %if.then5.i ], [ %add13.i, %if.end7.i ]
  %cmp37 = icmp sgt i32 %retval.0.i, 15
  br i1 %cmp37, label %if.then39, label %if.end55

if.then39:                                        ; preds = %digitval.exit
  %tobool41.not = icmp eq ptr %endp, null
  br i1 %tobool41.not, label %cleanup, label %cleanup.sink.split

if.else:                                          ; preds = %if.then28
  %spec.store.select = select i1 %cmp20, i32 8, i32 %base
  br label %if.end55

if.else50:                                        ; preds = %if.end18
  %conv19 = zext i8 %3 to i32
  %spec.store.select110 = select i1 %cmp20, i32 10, i32 %base
  br label %if.end55

if.end55:                                         ; preds = %if.else50, %if.else, %digitval.exit, %if.end7.i, %if.then33
  %base.addr.0 = phi i32 [ %spec.store.select, %if.else ], [ %spec.store.select110, %if.else50 ], [ 16, %digitval.exit ], [ 16, %if.end7.i ], [ 16, %if.then33 ]
  %p.2 = phi ptr [ %incdec.ptr29, %if.else ], [ %p.1, %if.else50 ], [ %incdec.ptr34, %digitval.exit ], [ %incdec.ptr34, %if.end7.i ], [ %incdec.ptr34, %if.then33 ]
  %c.0 = phi i32 [ %conv30, %if.else ], [ %conv19, %if.else50 ], [ %conv35, %digitval.exit ], [ %conv35, %if.end7.i ], [ %conv35, %if.then33 ]
  %10 = add nsw i32 %c.0, -48
  %or.cond.i167 = icmp ult i32 %10, 10
  br i1 %or.cond.i167, label %digitval.exit188, label %if.end.i168

if.end.i168:                                      ; preds = %if.end55
  %11 = add nsw i32 %c.0, -97
  %or.cond15.i169 = icmp ult i32 %11, 26
  br i1 %or.cond15.i169, label %if.then5.i186, label %if.end7.i170

if.end7.i170:                                     ; preds = %if.end.i168
  %12 = add nsw i32 %c.0, -91
  %or.cond285 = icmp ult i32 %12, -26
  br i1 %or.cond285, label %if.then63, label %if.end7.i181

if.then5.i186:                                    ; preds = %if.end.i168
  %add.i187 = add nsw i32 %c.0, -87
  br label %digitval.exit188

if.end7.i181:                                     ; preds = %if.end7.i170
  %add13.i183 = add nsw i32 %c.0, -55
  br label %digitval.exit188

digitval.exit188:                                 ; preds = %if.end7.i181, %if.then5.i186, %if.end55
  %retval.0.i185 = phi i32 [ %add.i187, %if.then5.i186 ], [ %add13.i183, %if.end7.i181 ], [ %10, %if.end55 ]
  %cmp61.not = icmp slt i32 %retval.0.i185, %base.addr.0
  br i1 %cmp61.not, label %for.cond.preheader, label %if.then63

for.cond.preheader:                               ; preds = %digitval.exit188
  %conv77 = zext nneg i32 %base.addr.0 to i64
  br label %for.cond

if.then63:                                        ; preds = %digitval.exit188, %if.end7.i170
  %tobool64.not = icmp eq ptr %endp, null
  br i1 %tobool64.not, label %cleanup, label %cleanup.sink.split

for.cond:                                         ; preds = %digitval.exit254, %for.cond.preheader
  %p.3 = phi ptr [ %incdec.ptr74276303307, %digitval.exit254 ], [ %p.2, %for.cond.preheader ]
  %c.1 = phi i32 [ %conv99, %digitval.exit254 ], [ %c.0, %for.cond.preheader ]
  %y.0 = phi i64 [ %add, %digitval.exit254 ], [ 0, %for.cond.preheader ]
  %13 = add nsw i32 %c.1, -48
  %or.cond.i189 = icmp ult i32 %13, 10
  br i1 %or.cond.i189, label %digitval.exit210.thread, label %if.end.i190

if.end.i190:                                      ; preds = %for.cond
  %14 = add nsw i32 %c.1, -97
  %or.cond15.i191 = icmp ult i32 %14, 26
  br i1 %or.cond15.i191, label %digitval.exit210, label %if.end7.i192

if.end7.i192:                                     ; preds = %if.end.i190
  %15 = add nsw i32 %c.1, -65
  %or.cond286 = icmp ult i32 %15, 26
  %add13.i205 = add nsw i32 %c.1, -55
  %cmp72296 = icmp slt i32 %add13.i205, %base.addr.0
  %or.cond312 = select i1 %or.cond286, i1 %cmp72296, i1 false
  br i1 %or.cond312, label %digitval.exit221, label %for.end

digitval.exit210:                                 ; preds = %if.end.i190
  %add.i209 = add nsw i32 %c.1, -87
  %cmp72 = icmp slt i32 %add.i209, %base.addr.0
  br i1 %cmp72, label %digitval.exit221, label %for.end

digitval.exit210.thread:                          ; preds = %for.cond
  %cmp72274 = icmp ult i32 %13, %base.addr.0
  br i1 %cmp72274, label %digitval.exit221.thread, label %for.end

digitval.exit221:                                 ; preds = %digitval.exit210, %if.end7.i192
  %.sink = phi i32 [ -87, %digitval.exit210 ], [ -55, %if.end7.i192 ]
  %add.i220 = add nsw i32 %c.1, %.sink
  %incdec.ptr74276 = getelementptr inbounds i8, ptr %p.3, i64 1
  %conv76 = sext i32 %add.i220 to i64
  %sub = sub i64 %lim, %conv76
  %div = udiv i64 %sub, %conv77
  %cmp78 = icmp ugt i64 %y.0, %div
  br i1 %cmp78, label %while.cond81.preheader, label %if.end95

digitval.exit221.thread:                          ; preds = %digitval.exit210.thread
  %incdec.ptr74276298 = getelementptr inbounds i8, ptr %p.3, i64 1
  %conv76299 = zext nneg i32 %13 to i64
  %sub300 = sub i64 %lim, %conv76299
  %div301 = udiv i64 %sub300, %conv77
  %cmp78302 = icmp ugt i64 %y.0, %div301
  br i1 %cmp78302, label %while.cond81.preheader, label %digitval.exit254

while.cond81.preheader:                           ; preds = %digitval.exit221.thread, %digitval.exit221
  %p.4.ph = phi ptr [ %incdec.ptr74276, %digitval.exit221 ], [ %incdec.ptr74276298, %digitval.exit221.thread ]
  br label %while.cond81

while.cond81:                                     ; preds = %while.body92, %while.cond81.preheader
  %p.4 = phi ptr [ %incdec.ptr93, %while.body92 ], [ %p.4.ph, %while.cond81.preheader ]
  %16 = load i8, ptr %p.4, align 1, !tbaa !4
  %conv82 = zext i8 %16 to i32
  %17 = add nsw i32 %conv82, -48
  %or.cond.i222 = icmp ult i32 %17, 10
  br i1 %or.cond.i222, label %digitval.exit243, label %if.end.i223

if.end.i223:                                      ; preds = %while.cond81
  %18 = add nsw i32 %conv82, -97
  %or.cond15.i224 = icmp ult i32 %18, 26
  br i1 %or.cond15.i224, label %if.then5.i241, label %if.end7.i225

if.end7.i225:                                     ; preds = %if.end.i223
  %19 = add nsw i32 %conv82, -65
  %or.cond16.i226 = icmp ult i32 %19, 26
  %cmp84 = icmp ugt i8 %16, 54
  %or.cond287 = and i1 %cmp84, %or.cond16.i226
  br i1 %or.cond287, label %if.end7.i236, label %for.end

if.then5.i241:                                    ; preds = %if.end.i223
  %add.i242 = add nsw i32 %conv82, -87
  br label %digitval.exit243

if.end7.i236:                                     ; preds = %if.end7.i225
  %add13.i238 = add nsw i32 %conv82, -55
  br label %digitval.exit243

digitval.exit243:                                 ; preds = %if.end7.i236, %if.then5.i241, %while.cond81
  %retval.0.i240 = phi i32 [ %add.i242, %if.then5.i241 ], [ %add13.i238, %if.end7.i236 ], [ %17, %while.cond81 ]
  %cmp89 = icmp slt i32 %retval.0.i240, %base.addr.0
  br i1 %cmp89, label %while.body92, label %for.end

while.body92:                                     ; preds = %digitval.exit243
  %incdec.ptr93 = getelementptr inbounds i8, ptr %p.4, i64 1
  br label %while.cond81, !llvm.loop !27

if.end95:                                         ; preds = %digitval.exit221
  br i1 %or.cond.i189, label %digitval.exit254, label %if.end.i245

if.end.i245:                                      ; preds = %if.end95
  %20 = add nsw i32 %c.1, -97
  %or.cond15.i246 = icmp ult i32 %20, 26
  br i1 %or.cond15.i246, label %if.then5.i252, label %if.end7.i247

if.then5.i252:                                    ; preds = %if.end.i245
  %add.i253 = add nsw i32 %c.1, -87
  br label %digitval.exit254

if.end7.i247:                                     ; preds = %if.end.i245
  %21 = add nsw i32 %c.1, -65
  %or.cond16.i248 = icmp ult i32 %21, 26
  %add13.i249 = add nsw i32 %c.1, -55
  %spec.select.i250 = select i1 %or.cond16.i248, i32 %add13.i249, i32 -1
  br label %digitval.exit254

digitval.exit254:                                 ; preds = %if.end7.i247, %if.then5.i252, %if.end95, %digitval.exit221.thread
  %incdec.ptr74276303307 = phi ptr [ %incdec.ptr74276, %if.then5.i252 ], [ %incdec.ptr74276, %if.end7.i247 ], [ %incdec.ptr74276, %if.end95 ], [ %incdec.ptr74276298, %digitval.exit221.thread ]
  %retval.0.i251 = phi i32 [ %add.i253, %if.then5.i252 ], [ %spec.select.i250, %if.end7.i247 ], [ %13, %if.end95 ], [ %13, %digitval.exit221.thread ]
  %mul308 = mul i64 %y.0, %conv77
  %conv98 = sext i32 %retval.0.i251 to i64
  %add = add i64 %mul308, %conv98
  %22 = load i8, ptr %incdec.ptr74276303307, align 1, !tbaa !4
  %conv99 = zext i8 %22 to i32
  br label %for.cond, !llvm.loop !28

for.end:                                          ; preds = %digitval.exit243, %if.end7.i225, %digitval.exit210.thread, %digitval.exit210, %if.end7.i192
  %p.5 = phi ptr [ %p.4, %digitval.exit243 ], [ %p.4, %if.end7.i225 ], [ %p.3, %digitval.exit210 ], [ %p.3, %digitval.exit210.thread ], [ %p.3, %if.end7.i192 ]
  %y.1 = phi i64 [ %lim, %digitval.exit243 ], [ %lim, %if.end7.i225 ], [ %y.0, %digitval.exit210 ], [ %y.0, %digitval.exit210.thread ], [ %y.0, %if.end7.i192 ]
  %tobool100.not = icmp eq ptr %endp, null
  br i1 %tobool100.not, label %cleanup, label %cleanup.sink.split

cleanup.sink.split:                               ; preds = %for.end, %if.then63, %if.then39, %if.then
  %p.5.sink = phi ptr [ %s, %if.then ], [ %incdec.ptr29, %if.then39 ], [ %s, %if.then63 ], [ %p.5, %for.end ]
  %neg.0.sink.ph = phi i32 [ 0, %if.then ], [ 0, %if.then39 ], [ 0, %if.then63 ], [ %neg.0, %for.end ]
  %retval.0.ph = phi i64 [ 0, %if.then ], [ 0, %if.then39 ], [ 0, %if.then63 ], [ %y.1, %for.end ]
  store ptr %p.5.sink, ptr %endp, align 8, !tbaa !18
  br label %cleanup

cleanup:                                          ; preds = %cleanup.sink.split, %for.end, %if.then63, %if.then39, %if.then
  %neg.0.sink = phi i32 [ 0, %if.then ], [ 0, %if.then39 ], [ 0, %if.then63 ], [ %neg.0, %for.end ], [ %neg.0.sink.ph, %cleanup.sink.split ]
  %retval.0 = phi i64 [ 0, %if.then ], [ 0, %if.then39 ], [ 0, %if.then63 ], [ %y.1, %for.end ], [ %retval.0.ph, %cleanup.sink.split ]
  store i32 %neg.0.sink, ptr %negative, align 4, !tbaa !24
  ret i64 %retval.0
}

; Function Attrs: nounwind
define dso_local i64 @strtoull(ptr noundef %s, ptr noundef %endp, i32 noundef %base) local_unnamed_addr #2 {
entry:
  %neg = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg) #13
  %call = call fastcc i64 @scan(ptr noundef %s, ptr noundef %endp, i32 noundef %base, i64 noundef -1, ptr noundef nonnull %neg) #14
  %0 = load i32, ptr %neg, align 4, !tbaa !24
  %tobool.not = icmp eq i32 %0, 0
  %sub = sub i64 0, %call
  %cond = select i1 %tobool.not, i64 %call, i64 %sub
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg) #13
  ret i64 %cond
}

; Function Attrs: nounwind
define dso_local i32 @strtol(ptr noundef %s, ptr noundef %endp, i32 noundef %base) local_unnamed_addr #2 {
entry:
  %neg = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg) #13
  %call = call fastcc i64 @scan(ptr noundef %s, ptr noundef %endp, i32 noundef %base, i64 noundef 2147483647, ptr noundef nonnull %neg) #14
  %0 = load i32, ptr %neg, align 4, !tbaa !24
  %tobool.not = icmp eq i32 %0, 0
  %conv = trunc i64 %call to i32
  %sub = sub nsw i32 0, %conv
  %cond = select i1 %tobool.not, i32 %conv, i32 %sub
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg) #13
  ret i32 %cond
}

; Function Attrs: nounwind
define dso_local i32 @strtoul(ptr noundef %s, ptr noundef %endp, i32 noundef %base) local_unnamed_addr #2 {
entry:
  %neg = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg) #13
  %call = call fastcc i64 @scan(ptr noundef %s, ptr noundef %endp, i32 noundef %base, i64 noundef 4294967295, ptr noundef nonnull %neg) #14
  %0 = load i32, ptr %neg, align 4, !tbaa !24
  %tobool.not = icmp eq i32 %0, 0
  %1 = trunc i64 %call to i32
  %conv = sub i32 0, %1
  %cond = select i1 %tobool.not, i32 %1, i32 %conv
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg) #13
  ret i32 %cond
}

; Function Attrs: nounwind
define dso_local i32 @atoi(ptr noundef %s) local_unnamed_addr #2 {
entry:
  %neg.i = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg.i) #13
  %call.i = call fastcc i64 @scan(ptr noundef %s, ptr noundef null, i32 noundef 10, i64 noundef 2147483647, ptr noundef nonnull %neg.i) #14
  %0 = load i32, ptr %neg.i, align 4, !tbaa !24
  %tobool.not.i = icmp eq i32 %0, 0
  %conv.i = trunc i64 %call.i to i32
  %sub.i = sub nsw i32 0, %conv.i
  %cond.i = select i1 %tobool.not.i, i32 %conv.i, i32 %sub.i
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg.i) #13
  ret i32 %cond.i
}

; Function Attrs: nounwind
define dso_local i32 @atol(ptr noundef %s) local_unnamed_addr #2 {
entry:
  %neg.i = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg.i) #13
  %call.i = call fastcc i64 @scan(ptr noundef %s, ptr noundef null, i32 noundef 10, i64 noundef 2147483647, ptr noundef nonnull %neg.i) #14
  %0 = load i32, ptr %neg.i, align 4, !tbaa !24
  %tobool.not.i = icmp eq i32 %0, 0
  %conv.i = trunc i64 %call.i to i32
  %sub.i = sub nsw i32 0, %conv.i
  %cond.i = select i1 %tobool.not.i, i32 %conv.i, i32 %sub.i
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg.i) #13
  ret i32 %cond.i
}

; Function Attrs: nounwind
define dso_local i64 @atoll(ptr noundef %s) local_unnamed_addr #2 {
entry:
  %neg.i = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %neg.i) #13
  %call.i = call fastcc i64 @scan(ptr noundef %s, ptr noundef null, i32 noundef 10, i64 noundef 9223372036854775807, ptr noundef nonnull %neg.i) #14
  %0 = load i32, ptr %neg.i, align 4, !tbaa !24
  %tobool.not.i = icmp eq i32 %0, 0
  %sub.i = sub nsw i64 0, %call.i
  %cond.i = select i1 %tobool.not.i, i64 %call.i, i64 %sub.i
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %neg.i) #13
  ret i64 %cond.i
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @abs(i32 noundef %a) local_unnamed_addr #0 {
entry:
  %cond = tail call i32 @llvm.abs.i32(i32 %a, i1 false)
  ret i32 %cond
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.abs.i32(i32, i1 immarg) #7

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i32 @labs(i32 noundef %a) local_unnamed_addr #0 {
entry:
  %cond = tail call i32 @llvm.abs.i32(i32 %a, i1 false)
  ret i32 %cond
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i64 @llabs(i64 noundef %a) local_unnamed_addr #0 {
entry:
  %cond = tail call i64 @llvm.abs.i64(i64 %a, i1 false)
  ret i64 %cond
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.abs.i64(i64, i1 immarg) #7

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local i64 @div(i32 noundef %num, i32 noundef %den) local_unnamed_addr #0 {
entry:
  %div = sdiv i32 %num, %den
  %rem = srem i32 %num, %den
  %retval.sroa.2.0.insert.ext = zext i32 %rem to i64
  %retval.sroa.2.0.insert.shift = shl nuw i64 %retval.sroa.2.0.insert.ext, 32
  %retval.sroa.0.0.insert.ext = zext i32 %div to i64
  %retval.sroa.0.0.insert.insert = or disjoint i64 %retval.sroa.2.0.insert.shift, %retval.sroa.0.0.insert.ext
  ret i64 %retval.sroa.0.0.insert.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local i64 @ldiv(i32 noundef %num, i32 noundef %den) local_unnamed_addr #0 {
entry:
  %div = sdiv i32 %num, %den
  %rem = srem i32 %num, %den
  %retval.sroa.2.0.insert.ext = zext i32 %rem to i64
  %retval.sroa.2.0.insert.shift = shl nuw i64 %retval.sroa.2.0.insert.ext, 32
  %retval.sroa.0.0.insert.ext = zext i32 %div to i64
  %retval.sroa.0.0.insert.insert = or disjoint i64 %retval.sroa.2.0.insert.shift, %retval.sroa.0.0.insert.ext
  ret i64 %retval.sroa.0.0.insert.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define dso_local void @lldiv(ptr dead_on_unwind noalias nocapture writable writeonly sret(%struct.lldiv_t) align 8 %agg.result, i64 noundef %num, i64 noundef %den) local_unnamed_addr #8 {
entry:
  %div = sdiv i64 %num, %den
  store i64 %div, ptr %agg.result, align 8, !tbaa !29
  %rem = srem i64 %num, %den
  %rem1 = getelementptr inbounds %struct.lldiv_t, ptr %agg.result, i64 0, i32 1
  store i64 %rem, ptr %rem1, align 8, !tbaa !31
  ret void
}

; Function Attrs: nounwind
define dso_local void @qsort(ptr noundef %base, i64 noundef %n, i64 noundef %width, ptr nocapture noundef readonly %cmp) local_unnamed_addr #2 {
entry:
  %tmp.i = alloca [64 x i8], align 16
  %tobool = icmp ne i64 %width, 0
  %cmp1 = icmp ugt i64 %n, 1
  %or.cond = and i1 %cmp1, %tobool
  br i1 %or.cond, label %for.cond2.preheader.lr.ph.i, label %if.end

for.cond2.preheader.lr.ph.i:                      ; preds = %entry
  call void @llvm.lifetime.start.p0(i64 64, ptr nonnull %tmp.i) #13
  br label %for.cond2.preheader.i

for.cond2.preheader.i:                            ; preds = %for.inc25.i, %for.cond2.preheader.lr.ph.i
  %i.056.i = phi i64 [ 1, %for.cond2.preheader.lr.ph.i ], [ %inc.i, %for.inc25.i ]
  br label %for.body4.i

for.cond2.loopexit.i:                             ; preds = %for.body10.i
  %cmp3.not.i = icmp eq i64 %sub.i, 0
  br i1 %cmp3.not.i, label %for.inc25.i, label %for.body4.i, !llvm.loop !32

for.body4.i:                                      ; preds = %for.cond2.loopexit.i, %for.cond2.preheader.i
  %j.054.i = phi i64 [ %i.056.i, %for.cond2.preheader.i ], [ %sub.i, %for.cond2.loopexit.i ]
  %sub.i = add i64 %j.054.i, -1
  %mul.i = mul i64 %sub.i, %width
  %add.ptr.i = getelementptr inbounds i8, ptr %base, i64 %mul.i
  %mul5.i = mul i64 %j.054.i, %width
  %add.ptr6.i = getelementptr inbounds i8, ptr %base, i64 %mul5.i
  %call.i = call i32 %cmp(ptr noundef %add.ptr.i, ptr noundef %add.ptr6.i) #12
  %cmp7.i = icmp slt i32 %call.i, 1
  br i1 %cmp7.i, label %for.inc25.i, label %for.body10.i

for.body10.i:                                     ; preds = %for.body10.i, %for.body4.i
  %k.053.i = phi i64 [ %add.i, %for.body10.i ], [ 0, %for.body4.i ]
  %sub11.i = sub i64 %width, %k.053.i
  %cond.i = call i64 @llvm.umin.i64(i64 %sub11.i, i64 64)
  %add.ptr14.i = getelementptr inbounds i8, ptr %add.ptr.i, i64 %k.053.i
  %call15.i = call ptr @memcpy(ptr noundef nonnull %tmp.i, ptr noundef %add.ptr14.i, i64 noundef %cond.i) #12
  %add.ptr16.i = getelementptr inbounds i8, ptr %add.ptr6.i, i64 %k.053.i
  %call18.i = call ptr @memcpy(ptr noundef %add.ptr14.i, ptr noundef %add.ptr16.i, i64 noundef %cond.i) #12
  %call21.i = call ptr @memcpy(ptr noundef %add.ptr16.i, ptr noundef nonnull %tmp.i, i64 noundef %cond.i) #12
  %add.i = add i64 %k.053.i, 64
  %cmp9.i = icmp ult i64 %add.i, %width
  br i1 %cmp9.i, label %for.body10.i, label %for.cond2.loopexit.i, !llvm.loop !33

for.inc25.i:                                      ; preds = %for.body4.i, %for.cond2.loopexit.i
  %inc.i = add nuw i64 %i.056.i, 1
  %exitcond.not.i = icmp eq i64 %inc.i, %n
  br i1 %exitcond.not.i, label %isort.exit, label %for.cond2.preheader.i, !llvm.loop !34

isort.exit:                                       ; preds = %for.inc25.i
  call void @llvm.lifetime.end.p0(i64 64, ptr nonnull %tmp.i) #13
  br label %if.end

if.end:                                           ; preds = %isort.exit, %entry
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.umin.i64(i64, i64) #7

; Function Attrs: nounwind
define dso_local noundef ptr @bsearch(ptr noundef %key, ptr noundef %base, i64 noundef %nel, i64 noundef %width, ptr nocapture noundef readonly %cmp) local_unnamed_addr #2 {
entry:
  %cmp1.not25 = icmp eq i64 %nel, 0
  br i1 %cmp1.not25, label %return, label %while.body

while.body:                                       ; preds = %cleanup, %entry
  %nel.addr.027 = phi i64 [ %nel.addr.2, %cleanup ], [ %nel, %entry ]
  %base.addr.026 = phi ptr [ %base.addr.2, %cleanup ], [ %base, %entry ]
  %div19 = lshr i64 %nel.addr.027, 1
  %mul = mul i64 %div19, %width
  %add.ptr = getelementptr inbounds i8, ptr %base.addr.026, i64 %mul
  %call = tail call i32 %cmp(ptr noundef %key, ptr noundef %add.ptr) #12
  %cmp2 = icmp slt i32 %call, 0
  br i1 %cmp2, label %cleanup, label %if.else

if.else:                                          ; preds = %while.body
  %cmp4.not = icmp eq i32 %call, 0
  br i1 %cmp4.not, label %return, label %if.then5

if.then5:                                         ; preds = %if.else
  %add.ptr6 = getelementptr inbounds i8, ptr %add.ptr, i64 %width
  %add.neg = xor i64 %div19, -1
  %sub = add i64 %nel.addr.027, %add.neg
  br label %cleanup

cleanup:                                          ; preds = %if.then5, %while.body
  %base.addr.2 = phi ptr [ %add.ptr6, %if.then5 ], [ %base.addr.026, %while.body ]
  %nel.addr.2 = phi i64 [ %sub, %if.then5 ], [ %div19, %while.body ]
  %cmp1.not = icmp eq i64 %nel.addr.2, 0
  br i1 %cmp1.not, label %return, label %while.body

return:                                           ; preds = %cleanup, %if.else, %entry
  %retval.2 = phi ptr [ null, %entry ], [ %add.ptr, %if.else ], [ null, %cleanup ]
  ret ptr %retval.2
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, argmem: none, inaccessiblemem: none)
define dso_local i32 @rand() local_unnamed_addr #9 {
entry:
  %0 = load i64, ptr @rng_state, align 8, !tbaa !11
  %mul = mul i64 %0, 6364136223846793005
  %add = add i64 %mul, 1
  store i64 %add, ptr @rng_state, align 8, !tbaa !11
  %shr = lshr i64 %add, 33
  %conv = trunc i64 %shr to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none)
define dso_local void @srand(i32 noundef %seed) local_unnamed_addr #10 {
entry:
  %conv = zext i32 %seed to i64
  store i64 %conv, ptr @rng_state, align 8, !tbaa !11
  ret void
}

; Function Attrs: nounwind
define dso_local i32 @vsnprintf(ptr noundef %dst, i64 noundef %n, ptr noundef %fmt, ptr noundef %ap) local_unnamed_addr #2 {
entry:
  %ap.addr = alloca ptr, align 8
  %o = alloca %struct.out, align 8
  store ptr %ap, ptr %ap.addr, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %o) #13
  store ptr %dst, ptr %o, align 8, !tbaa !35
  %cap = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 1
  store i64 %n, ptr %cap, align 8, !tbaa !37
  %len = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  store i64 0, ptr %len, align 8, !tbaa !38
  %call = call fastcc i32 @finish(ptr noundef nonnull %o, ptr noundef %fmt, ptr noundef nonnull %ap.addr) #14
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %o) #13
  ret i32 %call
}

; Function Attrs: nounwind
define internal fastcc i32 @finish(ptr noundef %o, ptr noundef %fmt, ptr noundef %ap) unnamed_addr #2 {
entry:
  %big.i.i = alloca [144 x i32], align 16
  %buf.i524.i = alloca [24 x i8], align 16
  %ebuf.i525.i = alloca [16 x i8], align 16
  %buf.i.i = alloca [15 x i8], align 1
  %ebuf.i.i = alloca [16 x i8], align 16
  %buf.i = alloca [24 x i8], align 16
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %buf.i) #13
  %invariant.gep.i = getelementptr i8, ptr %ebuf.i.i, i64 -1
  %0 = load i8, ptr %fmt, align 1, !tbaa !4
  %tobool.not673694.i = icmp eq i8 %0, 0
  br i1 %tobool.not673694.i, label %printf_core.exit, label %for.cond1.preheader.lr.ph.lr.ph.i

for.cond1.preheader.lr.ph.lr.ph.i:                ; preds = %entry
  %ebuf.i525.i73 = ptrtoint ptr %ebuf.i525.i to i64
  %buf.i524.i76 = ptrtoint ptr %buf.i524.i to i64
  %len.i.i = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  %cap.i.i = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 1
  %add.ptr114.i = getelementptr inbounds i8, ptr %buf.i, i64 24
  %add.ptr.i526.i = getelementptr inbounds i8, ptr %ebuf.i525.i, i64 16
  %add.ptr.i829.i.i = getelementptr inbounds i32, ptr %big.i.i, i64 1
  %sub.ptr.lhs.cast1170.i.i = ptrtoint ptr %big.i.i to i64
  %sub.ptr.lhs.cast379.i.i = ptrtoint ptr %add.ptr.i526.i to i64
  %add.ptr501.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 9
  %sub.ptr.lhs.cast533.i.i = ptrtoint ptr %add.ptr501.ptr.i.i to i64
  %incdec.ptr436.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 8
  %incdec.ptr94.ptr.i.i = getelementptr inbounds i8, ptr %ebuf.i.i, i64 15
  %sub.ptr.rhs.cast.i.i = ptrtoint ptr %buf.i.i to i64
  %add.ptr233.i = getelementptr inbounds i8, ptr %buf.i, i64 23
  %sub.ptr.lhs.cast162.i = ptrtoint ptr %add.ptr114.i to i64
  %invariant.gep = getelementptr i8, ptr %buf.i524.i, i64 -1
  %invariant.gep95 = getelementptr i8, ptr %buf.i524.i, i64 -2
  %invariant.gep97 = getelementptr i8, ptr %buf.i524.i, i64 -3
  %invariant.gep99 = getelementptr i8, ptr %buf.i524.i, i64 -4
  %invariant.gep101 = getelementptr i8, ptr %buf.i524.i, i64 -5
  %invariant.gep103 = getelementptr i8, ptr %buf.i524.i, i64 -6
  br label %for.cond1.preheader.lr.ph.i

for.cond1.preheader.lr.ph.i:                      ; preds = %for.cond.outer.backedge.i, %for.cond1.preheader.lr.ph.lr.ph.i
  %1 = phi i8 [ %0, %for.cond1.preheader.lr.ph.lr.ph.i ], [ %182, %for.cond.outer.backedge.i ]
  %l.0.ph697.i = phi i32 [ 0, %for.cond1.preheader.lr.ph.lr.ph.i ], [ %l.0.ph.be.i, %for.cond.outer.backedge.i ]
  %arg.sroa.0.0.ph696.i = phi i64 [ undef, %for.cond1.preheader.lr.ph.lr.ph.i ], [ %arg.sroa.0.0.ph.be.i, %for.cond.outer.backedge.i ]
  %s.0.ph695.i = phi ptr [ %fmt, %for.cond1.preheader.lr.ph.lr.ph.i ], [ %incdec.ptr98.i, %for.cond.outer.backedge.i ]
  br label %for.cond1.preheader.i

for.cond1.preheader.i:                            ; preds = %if.end11.i.i, %for.cond1.preheader.lr.ph.i
  %2 = phi i8 [ %1, %for.cond1.preheader.lr.ph.i ], [ %12, %if.end11.i.i ]
  %l.0675.i = phi i32 [ %l.0.ph697.i, %for.cond1.preheader.lr.ph.i ], [ %add.i, %if.end11.i.i ]
  %s.0674.i = phi ptr [ %s.0.ph695.i, %for.cond1.preheader.lr.ph.i ], [ %s.2.lcssa.i, %if.end11.i.i ]
  br label %for.cond1.i

for.cond1.i:                                      ; preds = %for.inc.i, %for.cond1.preheader.i
  %3 = phi i8 [ %.pr.i, %for.inc.i ], [ %2, %for.cond1.preheader.i ]
  %s.1.i = phi ptr [ %incdec.ptr.i, %for.inc.i ], [ %s.0674.i, %for.cond1.preheader.i ]
  switch i8 %3, label %for.inc.i [
    i8 37, label %land.rhs9.i
    i8 0, label %for.end18.i
  ]

for.inc.i:                                        ; preds = %for.cond1.i
  %incdec.ptr.i = getelementptr inbounds i8, ptr %s.1.i, i64 1
  %.pr.i = load i8, ptr %incdec.ptr.i, align 1, !tbaa !4
  br label %for.cond1.i, !llvm.loop !39

land.rhs9.i:                                      ; preds = %for.inc16.i, %for.cond1.i
  %z.0669.i = phi ptr [ %incdec.ptr17.i, %for.inc16.i ], [ %s.1.i, %for.cond1.i ]
  %s.2668.i = phi ptr [ %add.ptr.i, %for.inc16.i ], [ %s.1.i, %for.cond1.i ]
  %arrayidx10.i = getelementptr inbounds i8, ptr %s.2668.i, i64 1
  %4 = load i8, ptr %arrayidx10.i, align 1, !tbaa !4
  %cmp12.i = icmp eq i8 %4, 37
  br i1 %cmp12.i, label %for.inc16.i, label %for.end18.i

for.inc16.i:                                      ; preds = %land.rhs9.i
  %incdec.ptr17.i = getelementptr inbounds i8, ptr %z.0669.i, i64 1
  %add.ptr.i = getelementptr inbounds i8, ptr %s.2668.i, i64 2
  %5 = load i8, ptr %add.ptr.i, align 1, !tbaa !4
  %cmp7.i = icmp eq i8 %5, 37
  br i1 %cmp7.i, label %land.rhs9.i, label %for.end18.i, !llvm.loop !40

for.end18.i:                                      ; preds = %for.inc16.i, %land.rhs9.i, %for.cond1.i
  %s.2.lcssa.i = phi ptr [ %s.2668.i, %land.rhs9.i ], [ %add.ptr.i, %for.inc16.i ], [ %s.1.i, %for.cond1.i ]
  %z.0.lcssa.i = phi ptr [ %z.0669.i, %land.rhs9.i ], [ %incdec.ptr17.i, %for.inc16.i ], [ %s.1.i, %for.cond1.i ]
  %tobool19.not.i = icmp eq ptr %z.0.lcssa.i, %s.0674.i
  br i1 %tobool19.not.i, label %for.cond30.preheader.i, label %if.then20.i

for.cond30.preheader.i:                           ; preds = %for.end18.i
  %storemerge678.i = getelementptr inbounds i8, ptr %s.2.lcssa.i, i64 1
  %6 = load i8, ptr %storemerge678.i, align 1, !tbaa !4
  %conv31679.i = sext i8 %6 to i32
  %sub680.i = add nsw i32 %conv31679.i, -32
  %cmp32681.i = icmp ult i32 %sub680.i, 32
  br i1 %cmp32681.i, label %land.rhs34.i.preheader, label %for.end45.i

land.rhs34.i.preheader:                           ; preds = %for.cond30.preheader.i
  %shl.i23 = shl nuw i32 1, %sub680.i
  %and.i24 = and i32 %shl.i23, 75913
  %tobool37.not.i25 = icmp eq i32 %and.i24, 0
  br i1 %tobool37.not.i25, label %for.end45.i, label %for.cond30.i

if.then20.i:                                      ; preds = %for.end18.i
  %sub.ptr.rhs.cast.i = ptrtoint ptr %s.0674.i to i64
  %sub.ptr.lhs.cast.i = ptrtoint ptr %z.0.lcssa.i to i64
  %sub.ptr.sub.i = sub i64 %sub.ptr.lhs.cast.i, %sub.ptr.rhs.cast.i
  %7 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i.i = icmp eq ptr %7, null
  br i1 %tobool1.not.i.i, label %if.else.i.i, label %if.then2.i.i

if.then2.i.i:                                     ; preds = %if.then20.i
  %8 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %9 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i.i = icmp ult i64 %8, %9
  br i1 %cmp.i.i, label %if.then3.i.i, label %if.end11.i.i

if.then3.i.i:                                     ; preds = %if.then2.i.i
  %10 = xor i64 %8, -1
  %sub6.i.i = add i64 %9, %10
  %cond.i.i = call i64 @llvm.umin.i64(i64 %sub6.i.i, i64 %sub.ptr.sub.i)
  %add.ptr.i.i = getelementptr inbounds i8, ptr %7, i64 %8
  %call.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i.i, ptr noundef nonnull %s.0674.i, i64 noundef %cond.i.i) #12
  br label %if.end11.i.i

if.else.i.i:                                      ; preds = %if.then20.i
  call void @__msil_write(ptr noundef nonnull %s.0674.i, i64 noundef %sub.ptr.sub.i) #12
  br label %if.end11.i.i

if.end11.i.i:                                     ; preds = %if.else.i.i, %if.then3.i.i, %if.then2.i.i
  %11 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i.i = add i64 %11, %sub.ptr.sub.i
  store i64 %add.i.i, ptr %len.i.i, align 8, !tbaa !38
  %conv27.i = trunc i64 %sub.ptr.sub.i to i32
  %add.i = add nsw i32 %l.0675.i, %conv27.i
  %12 = load i8, ptr %s.2.lcssa.i, align 1, !tbaa !4
  %tobool.not.i = icmp eq i8 %12, 0
  br i1 %tobool.not.i, label %printf_core.exit, label %for.cond1.preheader.i

for.cond30.i:                                     ; preds = %land.rhs34.i, %land.rhs34.i.preheader
  %shl.i28 = phi i32 [ %shl.i, %land.rhs34.i ], [ %shl.i23, %land.rhs34.i.preheader ]
  %fl.0683.i27 = phi i32 [ %or.i, %land.rhs34.i ], [ 0, %land.rhs34.i.preheader ]
  %storemerge684.i26 = phi ptr [ %storemerge.i, %land.rhs34.i ], [ %storemerge678.i, %land.rhs34.i.preheader ]
  %or.i = or i32 %fl.0683.i27, %shl.i28
  %storemerge.i = getelementptr inbounds i8, ptr %storemerge684.i26, i64 1
  %13 = load i8, ptr %storemerge.i, align 1, !tbaa !4
  %conv31.i = sext i8 %13 to i32
  %sub.i = add nsw i32 %conv31.i, -32
  %cmp32.i = icmp ult i32 %sub.i, 32
  br i1 %cmp32.i, label %land.rhs34.i, label %for.end45.i, !llvm.loop !41

land.rhs34.i:                                     ; preds = %for.cond30.i
  %shl.i = shl nuw i32 1, %sub.i
  %and.i = and i32 %shl.i, 75913
  %tobool37.not.i = icmp eq i32 %and.i, 0
  br i1 %tobool37.not.i, label %for.end45.i, label %for.cond30.i, !llvm.loop !41

for.end45.i:                                      ; preds = %land.rhs34.i, %for.cond30.i, %land.rhs34.i.preheader, %for.cond30.preheader.i
  %.pn.lcssa.i = phi ptr [ %s.2.lcssa.i, %for.cond30.preheader.i ], [ %s.2.lcssa.i, %land.rhs34.i.preheader ], [ %storemerge684.i26, %land.rhs34.i ], [ %storemerge684.i26, %for.cond30.i ]
  %fl.0.lcssa.i = phi i32 [ 0, %for.cond30.preheader.i ], [ 0, %land.rhs34.i.preheader ], [ %or.i, %land.rhs34.i ], [ %or.i, %for.cond30.i ]
  %storemerge.lcssa.i = phi ptr [ %storemerge678.i, %for.cond30.preheader.i ], [ %storemerge678.i, %land.rhs34.i.preheader ], [ %storemerge.i, %land.rhs34.i ], [ %storemerge.i, %for.cond30.i ]
  %.lcssa.i = phi i8 [ %6, %for.cond30.preheader.i ], [ %6, %land.rhs34.i.preheader ], [ %13, %land.rhs34.i ], [ %13, %for.cond30.i ]
  %cmp47.i = icmp eq i8 %.lcssa.i, 42
  br i1 %cmp47.i, label %if.then49.i, label %if.else.i

if.then49.i:                                      ; preds = %for.end45.i
  %call.i = call i64 @__msil_va_i64(ptr noundef %ap) #12
  %conv50.i = trunc i64 %call.i to i32
  %incdec.ptr51.i = getelementptr inbounds i8, ptr %.pn.lcssa.i, i64 2
  %cmp52.i = icmp eq i32 %conv50.i, -2147483648
  br i1 %cmp52.i, label %printf_core.exit, label %if.end55.i

if.end55.i:                                       ; preds = %if.then49.i
  %cmp56.i = icmp slt i32 %conv50.i, 0
  br i1 %cmp56.i, label %if.then58.i, label %if.end67.i

if.then58.i:                                      ; preds = %if.end55.i
  %or59.i = or i32 %fl.0.lcssa.i, 8192
  %sub60.i = sub nsw i32 0, %conv50.i
  br label %if.end67.i

if.else.i:                                        ; preds = %for.end45.i
  %conv17.i.i = zext i8 %.lcssa.i to i32
  %call18.i.i = call i32 @isdigit(i32 noundef %conv17.i.i) #12
  %tobool.not19.i.i = icmp eq i32 %call18.i.i, 0
  br i1 %tobool.not19.i.i, label %if.end67.i, label %for.body.i.i

for.body.i.i:                                     ; preds = %for.inc.i.i, %if.else.i
  %i.021.i.i = phi i32 [ %i.1.i.i, %for.inc.i.i ], [ 0, %if.else.i ]
  %s.020.i.i = phi ptr [ %incdec.ptr.i.i, %for.inc.i.i ], [ %storemerge.lcssa.i, %if.else.i ]
  %cmp.i461.i = icmp sgt i32 %i.021.i.i, 214748364
  br i1 %cmp.i461.i, label %for.inc.i.i, label %lor.lhs.false.i.i

lor.lhs.false.i.i:                                ; preds = %for.body.i.i
  %14 = load i8, ptr %s.020.i.i, align 1, !tbaa !4
  %conv2.i.i = sext i8 %14 to i32
  %sub.i.i = add nsw i32 %conv2.i.i, -48
  %mul.i.i = mul nsw i32 %i.021.i.i, 10
  %sub3.i.i = sub nsw i32 2147483647, %mul.i.i
  %cmp4.i.i = icmp sgt i32 %sub.i.i, %sub3.i.i
  %add.i462.i = add nsw i32 %sub.i.i, %mul.i.i
  %spec.select.i.i = select i1 %cmp4.i.i, i32 -1, i32 %add.i462.i
  br label %for.inc.i.i

for.inc.i.i:                                      ; preds = %lor.lhs.false.i.i, %for.body.i.i
  %i.1.i.i = phi i32 [ -1, %for.body.i.i ], [ %spec.select.i.i, %lor.lhs.false.i.i ]
  %incdec.ptr.i.i = getelementptr inbounds i8, ptr %s.020.i.i, i64 1
  %15 = load i8, ptr %incdec.ptr.i.i, align 1, !tbaa !4
  %conv.i.i = zext i8 %15 to i32
  %call.i463.i = call i32 @isdigit(i32 noundef %conv.i.i) #12
  %tobool.not.i464.i = icmp eq i32 %call.i463.i, 0
  br i1 %tobool.not.i464.i, label %getint.exit.i, label %for.body.i.i, !llvm.loop !42

getint.exit.i:                                    ; preds = %for.inc.i.i
  %cmp63.i = icmp slt i32 %i.1.i.i, 0
  br i1 %cmp63.i, label %printf_core.exit, label %if.end67.i

if.end67.i:                                       ; preds = %getint.exit.i, %if.else.i, %if.then58.i, %if.end55.i
  %s.3.i = phi ptr [ %incdec.ptr51.i, %if.then58.i ], [ %incdec.ptr51.i, %if.end55.i ], [ %incdec.ptr.i.i, %getint.exit.i ], [ %storemerge.lcssa.i, %if.else.i ]
  %fl.1.i = phi i32 [ %or59.i, %if.then58.i ], [ %fl.0.lcssa.i, %if.end55.i ], [ %fl.0.lcssa.i, %getint.exit.i ], [ %fl.0.lcssa.i, %if.else.i ]
  %w.0.i = phi i32 [ %sub60.i, %if.then58.i ], [ %conv50.i, %if.end55.i ], [ %i.1.i.i, %getint.exit.i ], [ 0, %if.else.i ]
  %16 = load i8, ptr %s.3.i, align 1, !tbaa !4
  %cmp69.i = icmp eq i8 %16, 46
  br i1 %cmp69.i, label %land.lhs.true.i, label %if.end90.i

land.lhs.true.i:                                  ; preds = %if.end67.i
  %arrayidx71.i = getelementptr inbounds i8, ptr %s.3.i, i64 1
  %17 = load i8, ptr %arrayidx71.i, align 1, !tbaa !4
  %cmp73.i = icmp eq i8 %17, 42
  br i1 %cmp73.i, label %if.then75.i, label %if.then85.i

if.then75.i:                                      ; preds = %land.lhs.true.i
  %call76.i = call i64 @__msil_va_i64(ptr noundef %ap) #12
  %conv77.i = trunc i64 %call76.i to i32
  %add.ptr78.i = getelementptr inbounds i8, ptr %s.3.i, i64 2
  %cmp79.i = icmp sgt i32 %conv77.i, -1
  br label %if.end90.i

if.then85.i:                                      ; preds = %land.lhs.true.i
  %conv17.i465.i = zext i8 %17 to i32
  %call18.i466.i = call i32 @isdigit(i32 noundef %conv17.i465.i) #12
  %tobool.not19.i467.i = icmp eq i32 %call18.i466.i, 0
  br i1 %tobool.not19.i467.i, label %if.end90.i, label %for.body.i468.i

for.body.i468.i:                                  ; preds = %for.inc.i480.i, %if.then85.i
  %i.021.i469.i = phi i32 [ %i.1.i481.i, %for.inc.i480.i ], [ 0, %if.then85.i ]
  %s.020.i470.i = phi ptr [ %incdec.ptr.i482.i, %for.inc.i480.i ], [ %arrayidx71.i, %if.then85.i ]
  %cmp.i471.i = icmp sgt i32 %i.021.i469.i, 214748364
  br i1 %cmp.i471.i, label %for.inc.i480.i, label %lor.lhs.false.i472.i

lor.lhs.false.i472.i:                             ; preds = %for.body.i468.i
  %18 = load i8, ptr %s.020.i470.i, align 1, !tbaa !4
  %conv2.i473.i = sext i8 %18 to i32
  %sub.i474.i = add nsw i32 %conv2.i473.i, -48
  %mul.i475.i = mul nsw i32 %i.021.i469.i, 10
  %sub3.i476.i = sub nsw i32 2147483647, %mul.i475.i
  %cmp4.i477.i = icmp sgt i32 %sub.i474.i, %sub3.i476.i
  %add.i478.i = add nsw i32 %sub.i474.i, %mul.i475.i
  %spec.select.i479.i = select i1 %cmp4.i477.i, i32 -1, i32 %add.i478.i
  br label %for.inc.i480.i

for.inc.i480.i:                                   ; preds = %lor.lhs.false.i472.i, %for.body.i468.i
  %i.1.i481.i = phi i32 [ -1, %for.body.i468.i ], [ %spec.select.i479.i, %lor.lhs.false.i472.i ]
  %incdec.ptr.i482.i = getelementptr inbounds i8, ptr %s.020.i470.i, i64 1
  %19 = load i8, ptr %incdec.ptr.i482.i, align 1, !tbaa !4
  %conv.i483.i = zext i8 %19 to i32
  %call.i484.i = call i32 @isdigit(i32 noundef %conv.i483.i) #12
  %tobool.not.i485.i = icmp eq i32 %call.i484.i, 0
  br i1 %tobool.not.i485.i, label %if.end90.i, label %for.body.i468.i, !llvm.loop !42

if.end90.i:                                       ; preds = %for.inc.i480.i, %if.then85.i, %if.then75.i, %if.end67.i
  %s.4.i = phi ptr [ %add.ptr78.i, %if.then75.i ], [ %s.3.i, %if.end67.i ], [ %arrayidx71.i, %if.then85.i ], [ %incdec.ptr.i482.i, %for.inc.i480.i ]
  %p.0.i = phi i32 [ %conv77.i, %if.then75.i ], [ -1, %if.end67.i ], [ 0, %if.then85.i ], [ %i.1.i481.i, %for.inc.i480.i ]
  %xp.0.i = phi i1 [ %cmp79.i, %if.then75.i ], [ false, %if.end67.i ], [ true, %if.then85.i ], [ true, %for.inc.i480.i ]
  br label %do.body.i

do.body.i:                                        ; preds = %if.end96.i, %if.end90.i
  %s.5.i = phi ptr [ %s.4.i, %if.end90.i ], [ %incdec.ptr98.i, %if.end96.i ]
  %st.0.i = phi i32 [ 0, %if.end90.i ], [ %conv103.i, %if.end96.i ]
  %20 = load i8, ptr %s.5.i, align 1, !tbaa !4
  %conv91.i = sext i8 %20 to i32
  %21 = add nsw i32 %conv91.i, -123
  %cmp93.i = icmp ult i32 %21, -58
  br i1 %cmp93.i, label %printf_core.exit, label %if.end96.i

if.end96.i:                                       ; preds = %do.body.i
  %idxprom.i = zext nneg i32 %st.0.i to i64
  %incdec.ptr98.i = getelementptr inbounds i8, ptr %s.5.i, i64 1
  %conv99.i = sext i8 %20 to i64
  %sub100.i = add nsw i64 %conv99.i, -65
  %arrayidx102.i = getelementptr inbounds [8 x [58 x i8]], ptr @states, i64 0, i64 %idxprom.i, i64 %sub100.i
  %22 = load i8, ptr %arrayidx102.i, align 1, !tbaa !4
  %conv103.i = zext i8 %22 to i32
  %sub104.i = add nsw i32 %conv103.i, -1
  %cmp105.i = icmp ult i32 %sub104.i, 8
  br i1 %cmp105.i, label %do.body.i, label %do.end.i, !llvm.loop !43

do.end.i:                                         ; preds = %if.end96.i
  switch i8 %22, label %if.end113.fold.split.i [
    i8 0, label %printf_core.exit
    i8 27, label %if.end113.i
    i8 9, label %sw.bb.i.i
    i8 24, label %sw.bb1.i.i
    i8 10, label %sw.bb3.i.i
    i8 11, label %sw.bb3.i.i
    i8 13, label %sw.bb3.i.i
    i8 14, label %sw.bb3.i.i
    i8 19, label %sw.bb3.i.i
    i8 12, label %sw.bb3.i.i
    i8 20, label %sw.bb3.i.i
    i8 21, label %sw.bb3.i.i
    i8 22, label %sw.bb3.i.i
    i8 23, label %sw.bb3.i.i
    i8 17, label %sw.bb3.i.i
    i8 18, label %sw.bb3.i.i
    i8 15, label %sw.bb3.i.i
    i8 16, label %sw.bb3.i.i
    i8 25, label %sw.bb5.i.i
    i8 26, label %sw.bb5.i.i
  ]

sw.bb.i.i:                                        ; preds = %do.end.i
  %call.i490.i = call ptr @__msil_va_ptr(ptr noundef %ap) #12
  %23 = ptrtoint ptr %call.i490.i to i64
  br label %if.end113.i

sw.bb1.i.i:                                       ; preds = %do.end.i
  %call2.i.i = call ptr @__msil_va_ptr(ptr noundef %ap) #12
  %24 = ptrtoint ptr %call2.i.i to i64
  br label %if.end113.i

sw.bb3.i.i:                                       ; preds = %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i, %do.end.i
  %call4.i.i = call i64 @__msil_va_i64(ptr noundef %ap) #12
  switch i8 %22, label %if.end113.i [
    i8 17, label %sw.bb7.i.i
    i8 18, label %sw.bb9.i.i
    i8 15, label %sw.bb12.i.i
    i8 16, label %sw.bb15.i.i
    i8 10, label %sw.bb18.i.i
    i8 11, label %sw.bb21.i.i
    i8 13, label %sw.bb24.i.i
    i8 14, label %sw.bb27.i.i
  ]

sw.bb5.i.i:                                       ; preds = %do.end.i, %do.end.i
  %call6.i.i = call double @__msil_va_f64(ptr noundef %ap) #12
  %25 = bitcast double %call6.i.i to i64
  br label %if.end113.i

sw.bb7.i.i:                                       ; preds = %sw.bb3.i.i
  %sext58.i.i = shl i64 %call4.i.i, 56
  %conv8.i.i = ashr exact i64 %sext58.i.i, 56
  br label %if.end113.i

sw.bb9.i.i:                                       ; preds = %sw.bb3.i.i
  %conv11.i.i = and i64 %call4.i.i, 255
  br label %if.end113.i

sw.bb12.i.i:                                      ; preds = %sw.bb3.i.i
  %sext57.i.i = shl i64 %call4.i.i, 48
  %conv14.i.i = ashr exact i64 %sext57.i.i, 48
  br label %if.end113.i

sw.bb15.i.i:                                      ; preds = %sw.bb3.i.i
  %conv17.i489.i = and i64 %call4.i.i, 65535
  br label %if.end113.i

sw.bb18.i.i:                                      ; preds = %sw.bb3.i.i
  %sext56.i.i = shl i64 %call4.i.i, 32
  %conv20.i.i = ashr exact i64 %sext56.i.i, 32
  br label %if.end113.i

sw.bb21.i.i:                                      ; preds = %sw.bb3.i.i
  %conv23.i.i = and i64 %call4.i.i, 4294967295
  br label %if.end113.i

sw.bb24.i.i:                                      ; preds = %sw.bb3.i.i
  %sext.i.i = shl i64 %call4.i.i, 32
  %conv26.i.i = ashr exact i64 %sext.i.i, 32
  br label %if.end113.i

sw.bb27.i.i:                                      ; preds = %sw.bb3.i.i
  %conv29.i.i = and i64 %call4.i.i, 4294967295
  br label %if.end113.i

if.end113.fold.split.i:                           ; preds = %do.end.i
  br label %if.end113.i

if.end113.i:                                      ; preds = %if.end113.fold.split.i, %sw.bb27.i.i, %sw.bb24.i.i, %sw.bb21.i.i, %sw.bb18.i.i, %sw.bb15.i.i, %sw.bb12.i.i, %sw.bb9.i.i, %sw.bb7.i.i, %sw.bb5.i.i, %sw.bb3.i.i, %sw.bb1.i.i, %sw.bb.i.i, %do.end.i
  %arg.sroa.0.2.i = phi i64 [ %arg.sroa.0.0.ph696.i, %do.end.i ], [ %25, %sw.bb5.i.i ], [ %call4.i.i, %sw.bb3.i.i ], [ %conv29.i.i, %sw.bb27.i.i ], [ %conv26.i.i, %sw.bb24.i.i ], [ %conv23.i.i, %sw.bb21.i.i ], [ %conv20.i.i, %sw.bb18.i.i ], [ %conv17.i489.i, %sw.bb15.i.i ], [ %conv14.i.i, %sw.bb12.i.i ], [ %conv11.i.i, %sw.bb9.i.i ], [ %conv8.i.i, %sw.bb7.i.i ], [ %24, %sw.bb1.i.i ], [ %23, %sw.bb.i.i ], [ 0, %if.end113.fold.split.i ]
  %26 = load i8, ptr %s.5.i, align 1, !tbaa !4
  %conv116.i = sext i8 %26 to i32
  %tobool117.not.i = icmp ne i32 %st.0.i, 0
  %and119.i = and i32 %conv116.i, 15
  %cmp120.i = icmp eq i32 %and119.i, 3
  %or.cond457.i = and i1 %tobool117.not.i, %cmp120.i
  %and123.i = and i32 %conv116.i, -45
  %t.0.i = select i1 %or.cond457.i, i32 %and123.i, i32 %conv116.i
  %and125.i = and i32 %fl.1.i, 8192
  %tobool126.not.i = icmp eq i32 %and125.i, 0
  %and128.i = and i32 %fl.1.i, -65537
  %fl.2.i = select i1 %tobool126.not.i, i32 %fl.1.i, i32 %and128.i
  switch i32 %t.0.i, label %printf_core.exit [
    i32 110, label %sw.bb.i
    i32 112, label %sw.bb142.i
    i32 120, label %sw.bb146.i
    i32 88, label %sw.bb146.i
    i32 111, label %sw.bb156.i
    i32 100, label %sw.bb175.i
    i32 105, label %sw.bb175.i
    i32 117, label %sw.bb194.i
    i32 99, label %sw.bb231.i
    i32 115, label %sw.bb235.i
    i32 101, label %sw.bb262.i
    i32 102, label %sw.bb262.i
    i32 103, label %sw.bb262.i
    i32 69, label %sw.bb262.i
    i32 70, label %sw.bb262.i
    i32 71, label %sw.bb262.i
    i32 97, label %sw.bb262.i
    i32 65, label %sw.bb262.i
  ]

sw.bb.i:                                          ; preds = %if.end113.i
  %trunc.i = trunc i32 %st.0.i to i8
  switch i8 %trunc.i, label %for.cond.outer.backedge.i [
    i8 0, label %sw.bb130.i
    i8 1, label %sw.bb131.i
    i8 2, label %sw.bb132.i
    i8 3, label %sw.bb134.i
    i8 4, label %sw.bb136.i
    i8 6, label %sw.bb138.i
    i8 7, label %sw.bb140.i
  ]

sw.bb130.i:                                       ; preds = %sw.bb.i
  %27 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i32 %l.0675.i, ptr %27, align 4, !tbaa !24
  br label %for.cond.outer.backedge.i

sw.bb131.i:                                       ; preds = %sw.bb.i
  %28 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i32 %l.0675.i, ptr %28, align 4, !tbaa !44
  br label %for.cond.outer.backedge.i

sw.bb132.i:                                       ; preds = %sw.bb.i
  %conv133.i = sext i32 %l.0675.i to i64
  %29 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i64 %conv133.i, ptr %29, align 8, !tbaa !11
  br label %for.cond.outer.backedge.i

sw.bb134.i:                                       ; preds = %sw.bb.i
  %conv135.i = trunc i32 %l.0675.i to i16
  %30 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i16 %conv135.i, ptr %30, align 2, !tbaa !46
  br label %for.cond.outer.backedge.i

sw.bb136.i:                                       ; preds = %sw.bb.i
  %conv137.i = trunc i32 %l.0675.i to i8
  %31 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i8 %conv137.i, ptr %31, align 1, !tbaa !4
  br label %for.cond.outer.backedge.i

sw.bb138.i:                                       ; preds = %sw.bb.i
  %conv139.i = sext i32 %l.0675.i to i64
  %32 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i64 %conv139.i, ptr %32, align 8, !tbaa !11
  br label %for.cond.outer.backedge.i

sw.bb140.i:                                       ; preds = %sw.bb.i
  %conv141.i = sext i32 %l.0675.i to i64
  %33 = inttoptr i64 %arg.sroa.0.2.i to ptr
  store i64 %conv141.i, ptr %33, align 8, !tbaa !11
  br label %for.cond.outer.backedge.i

sw.bb142.i:                                       ; preds = %if.end113.i
  %cond.i = call i32 @llvm.smax.i32(i32 %p.0.i, i32 16)
  %or145.i = or i32 %fl.2.i, 8
  br label %sw.bb146.i

sw.bb146.i:                                       ; preds = %sw.bb142.i, %if.end113.i, %if.end113.i
  %fl.3.i = phi i32 [ %fl.2.i, %if.end113.i ], [ %fl.2.i, %if.end113.i ], [ %or145.i, %sw.bb142.i ]
  %p.1.i = phi i32 [ %p.0.i, %if.end113.i ], [ %p.0.i, %if.end113.i ], [ %cond.i, %sw.bb142.i ]
  %t.1.i = phi i32 [ %t.0.i, %if.end113.i ], [ %t.0.i, %if.end113.i ], [ 120, %sw.bb142.i ]
  %tobool.not5.i.i = icmp eq i64 %arg.sroa.0.2.i, 0
  br i1 %tobool.not5.i.i, label %ifmt_tail.i, label %for.body.lr.ph.i.i

for.body.lr.ph.i.i:                               ; preds = %sw.bb146.i
  %34 = trunc i32 %t.1.i to i8
  %35 = and i8 %34, 32
  br label %for.body.i491.i

for.body.i491.i:                                  ; preds = %for.body.i491.i, %for.body.lr.ph.i.i
  %x.addr.07.i.i = phi i64 [ %arg.sroa.0.2.i, %for.body.lr.ph.i.i ], [ %shr.i.i, %for.body.i491.i ]
  %s.addr.06.i.i = phi ptr [ %add.ptr114.i, %for.body.lr.ph.i.i ], [ %incdec.ptr.i492.i, %for.body.i491.i ]
  %and.i.i = and i64 %x.addr.07.i.i, 15
  %arrayidx.i.i = getelementptr inbounds [16 x i8], ptr @xdigits, i64 0, i64 %and.i.i
  %36 = load i8, ptr %arrayidx.i.i, align 1, !tbaa !4
  %conv1.i.i = or i8 %36, %35
  %incdec.ptr.i492.i = getelementptr inbounds i8, ptr %s.addr.06.i.i, i64 -1
  store i8 %conv1.i.i, ptr %incdec.ptr.i492.i, align 1, !tbaa !4
  %shr.i.i = lshr i64 %x.addr.07.i.i, 4
  %tobool.not.i493.i = icmp ult i64 %x.addr.07.i.i, 16
  br i1 %tobool.not.i493.i, label %fmt_x.exit.i, label %for.body.i491.i, !llvm.loop !48

fmt_x.exit.i:                                     ; preds = %for.body.i491.i
  %and151.i = and i32 %fl.3.i, 8
  %tobool152.not.i = icmp eq i32 %and151.i, 0
  br i1 %tobool152.not.i, label %ifmt_tail.i, label %if.then153.i

if.then153.i:                                     ; preds = %fmt_x.exit.i
  %shr.i = ashr i32 %t.1.i, 4
  %idx.ext.i = sext i32 %shr.i to i64
  %add.ptr154.i = getelementptr inbounds i8, ptr @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", i64 %idx.ext.i
  br label %ifmt_tail.i

sw.bb156.i:                                       ; preds = %if.end113.i
  %tobool.not4.i.i = icmp eq i64 %arg.sroa.0.2.i, 0
  br i1 %tobool.not4.i.i, label %fmt_o.exit.i, label %for.body.i494.i

for.body.i494.i:                                  ; preds = %for.body.i494.i, %sw.bb156.i
  %x.addr.06.i.i = phi i64 [ %shr.i497.i, %for.body.i494.i ], [ %arg.sroa.0.2.i, %sw.bb156.i ]
  %s.addr.05.i.i = phi ptr [ %incdec.ptr.i496.i, %for.body.i494.i ], [ %add.ptr114.i, %sw.bb156.i ]
  %37 = trunc i64 %x.addr.06.i.i to i8
  %38 = and i8 %37, 7
  %conv.i495.i = or disjoint i8 %38, 48
  %incdec.ptr.i496.i = getelementptr inbounds i8, ptr %s.addr.05.i.i, i64 -1
  store i8 %conv.i495.i, ptr %incdec.ptr.i496.i, align 1, !tbaa !4
  %shr.i497.i = lshr i64 %x.addr.06.i.i, 3
  %tobool.not.i498.i = icmp ult i64 %x.addr.06.i.i, 8
  br i1 %tobool.not.i498.i, label %fmt_o.exit.i, label %for.body.i494.i, !llvm.loop !49

fmt_o.exit.i:                                     ; preds = %for.body.i494.i, %sw.bb156.i
  %s.addr.0.lcssa.i499.i = phi ptr [ %add.ptr114.i, %sw.bb156.i ], [ %incdec.ptr.i496.i, %for.body.i494.i ]
  %and158.i = and i32 %fl.2.i, 8
  %tobool159.not.i = icmp eq i32 %and158.i, 0
  br i1 %tobool159.not.i, label %ifmt_tail.i, label %land.lhs.true160.i

land.lhs.true160.i:                               ; preds = %fmt_o.exit.i
  %conv161.i = sext i32 %p.0.i to i64
  %sub.ptr.rhs.cast163.i = ptrtoint ptr %s.addr.0.lcssa.i499.i to i64
  %sub.ptr.sub164.i = sub i64 %sub.ptr.lhs.cast162.i, %sub.ptr.rhs.cast163.i
  %cmp166.not.i = icmp slt i64 %sub.ptr.sub164.i, %conv161.i
  br i1 %cmp166.not.i, label %ifmt_tail.i, label %if.then168.i

if.then168.i:                                     ; preds = %land.lhs.true160.i
  %39 = trunc i64 %sub.ptr.sub164.i to i32
  %conv173.i = add i32 %39, 1
  br label %ifmt_tail.i

sw.bb175.i:                                       ; preds = %if.end113.i, %if.end113.i
  %cmp176.i = icmp slt i64 %arg.sroa.0.2.i, 0
  br i1 %cmp176.i, label %if.then178.i, label %if.else180.i

if.then178.i:                                     ; preds = %sw.bb175.i
  %sub179.i = sub i64 0, %arg.sroa.0.2.i
  br label %sw.bb194.i

if.else180.i:                                     ; preds = %sw.bb175.i
  %and181.i = and i32 %fl.2.i, 2048
  %tobool182.not.i = icmp eq i32 %and181.i, 0
  br i1 %tobool182.not.i, label %if.else185.i, label %sw.bb194.i

if.else185.i:                                     ; preds = %if.else180.i
  %and186.i = and i32 %fl.2.i, 1
  %tobool187.not.not.i = icmp eq i32 %and186.i, 0
  %"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@..i" = select i1 %tobool187.not.not.i, ptr @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", ptr getelementptr inbounds ([10 x i8], ptr @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", i64 0, i64 2)
  br label %sw.bb194.i

sw.bb194.i:                                       ; preds = %if.else185.i, %if.else180.i, %if.then178.i, %if.end113.i
  %arg.sroa.0.3.i = phi i64 [ %arg.sroa.0.2.i, %if.end113.i ], [ %sub179.i, %if.then178.i ], [ %arg.sroa.0.2.i, %if.else185.i ], [ %arg.sroa.0.2.i, %if.else180.i ]
  %prefix.0.i = phi ptr [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %if.end113.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %if.then178.i ], [ %"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@..i", %if.else185.i ], [ getelementptr inbounds ([10 x i8], ptr @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", i64 0, i64 1), %if.else180.i ]
  %pl.0.i = phi i32 [ 0, %if.end113.i ], [ 1, %if.then178.i ], [ %and186.i, %if.else185.i ], [ 1, %if.else180.i ]
  %cmp26.i.i = icmp ugt i64 %arg.sroa.0.3.i, 4294967295
  br i1 %cmp26.i.i, label %for.body.i506.i, label %for.end.i.i

for.body.i506.i:                                  ; preds = %for.body.i506.i, %sw.bb194.i
  %x.addr.028.i.i = phi i64 [ %div.i.i, %for.body.i506.i ], [ %arg.sroa.0.3.i, %sw.bb194.i ]
  %s.addr.027.i.i = phi ptr [ %incdec.ptr.i508.i, %for.body.i506.i ], [ %add.ptr114.i, %sw.bb194.i ]
  %rem.i.i = urem i64 %x.addr.028.i.i, 10
  %40 = trunc i64 %rem.i.i to i8
  %conv.i507.i = or disjoint i8 %40, 48
  %incdec.ptr.i508.i = getelementptr inbounds i8, ptr %s.addr.027.i.i, i64 -1
  store i8 %conv.i507.i, ptr %incdec.ptr.i508.i, align 1, !tbaa !4
  %div.i.i = udiv i64 %x.addr.028.i.i, 10
  %cmp.i509.i = icmp ugt i64 %x.addr.028.i.i, 42949672959
  br i1 %cmp.i509.i, label %for.body.i506.i, label %for.end.i.i, !llvm.loop !50

for.end.i.i:                                      ; preds = %for.body.i506.i, %sw.bb194.i
  %s.addr.0.lcssa.i500.i = phi ptr [ %add.ptr114.i, %sw.bb194.i ], [ %incdec.ptr.i508.i, %for.body.i506.i ]
  %x.addr.0.lcssa.i.i = phi i64 [ %arg.sroa.0.3.i, %sw.bb194.i ], [ %div.i.i, %for.body.i506.i ]
  %conv1.i501.i = trunc i64 %x.addr.0.lcssa.i.i to i32
  %cmp330.i.i = icmp ugt i32 %conv1.i501.i, 9
  br i1 %cmp330.i.i, label %for.body5.i.i, label %for.end12.i.i

for.body5.i.i:                                    ; preds = %for.body5.i.i, %for.end.i.i
  %y.032.i.i = phi i32 [ %div11.i.i, %for.body5.i.i ], [ %conv1.i501.i, %for.end.i.i ]
  %s.addr.131.i.i = phi ptr [ %incdec.ptr9.i.i, %for.body5.i.i ], [ %s.addr.0.lcssa.i500.i, %for.end.i.i ]
  %rem6.i.i = urem i32 %y.032.i.i, 10
  %41 = trunc i32 %rem6.i.i to i8
  %conv8.i505.i = or disjoint i8 %41, 48
  %incdec.ptr9.i.i = getelementptr inbounds i8, ptr %s.addr.131.i.i, i64 -1
  store i8 %conv8.i505.i, ptr %incdec.ptr9.i.i, align 1, !tbaa !4
  %div11.i.i = udiv i32 %y.032.i.i, 10
  %cmp3.i.i = icmp ugt i32 %y.032.i.i, 99
  br i1 %cmp3.i.i, label %for.body5.i.i, label %for.end12.i.i, !llvm.loop !51

for.end12.i.i:                                    ; preds = %for.body5.i.i, %for.end.i.i
  %s.addr.1.lcssa.i.i = phi ptr [ %s.addr.0.lcssa.i500.i, %for.end.i.i ], [ %incdec.ptr9.i.i, %for.body5.i.i ]
  %y.0.lcssa.i.i = phi i32 [ %conv1.i501.i, %for.end.i.i ], [ %div11.i.i, %for.body5.i.i ]
  %tobool.not.i502.i = icmp eq i32 %y.0.lcssa.i.i, 0
  br i1 %tobool.not.i502.i, label %ifmt_tail.i, label %if.then.i.i

if.then.i.i:                                      ; preds = %for.end12.i.i
  %42 = trunc i32 %y.0.lcssa.i.i to i8
  %conv14.i503.i = or disjoint i8 %42, 48
  %incdec.ptr15.i.i = getelementptr inbounds i8, ptr %s.addr.1.lcssa.i.i, i64 -1
  store i8 %conv14.i503.i, ptr %incdec.ptr15.i.i, align 1, !tbaa !4
  br label %ifmt_tail.i

ifmt_tail.i:                                      ; preds = %if.then.i.i, %for.end12.i.i, %if.then168.i, %land.lhs.true160.i, %fmt_o.exit.i, %if.then153.i, %fmt_x.exit.i, %sw.bb146.i
  %arg.sroa.0.4.i = phi i64 [ %arg.sroa.0.2.i, %fmt_o.exit.i ], [ %arg.sroa.0.2.i, %land.lhs.true160.i ], [ %arg.sroa.0.2.i, %if.then168.i ], [ %arg.sroa.0.2.i, %fmt_x.exit.i ], [ %arg.sroa.0.2.i, %if.then153.i ], [ %arg.sroa.0.3.i, %for.end12.i.i ], [ %arg.sroa.0.3.i, %if.then.i.i ], [ 0, %sw.bb146.i ]
  %a.0.i = phi ptr [ %s.addr.0.lcssa.i499.i, %fmt_o.exit.i ], [ %s.addr.0.lcssa.i499.i, %land.lhs.true160.i ], [ %s.addr.0.lcssa.i499.i, %if.then168.i ], [ %incdec.ptr.i492.i, %fmt_x.exit.i ], [ %incdec.ptr.i492.i, %if.then153.i ], [ %s.addr.1.lcssa.i.i, %for.end12.i.i ], [ %incdec.ptr15.i.i, %if.then.i.i ], [ %add.ptr114.i, %sw.bb146.i ]
  %fl.4.i = phi i32 [ %fl.2.i, %fmt_o.exit.i ], [ %fl.2.i, %land.lhs.true160.i ], [ %fl.2.i, %if.then168.i ], [ %fl.3.i, %fmt_x.exit.i ], [ %fl.3.i, %if.then153.i ], [ %fl.2.i, %for.end12.i.i ], [ %fl.2.i, %if.then.i.i ], [ %fl.3.i, %sw.bb146.i ]
  %p.2.i = phi i32 [ %p.0.i, %fmt_o.exit.i ], [ %p.0.i, %land.lhs.true160.i ], [ %conv173.i, %if.then168.i ], [ %p.1.i, %fmt_x.exit.i ], [ %p.1.i, %if.then153.i ], [ %p.0.i, %for.end12.i.i ], [ %p.0.i, %if.then.i.i ], [ %p.1.i, %sw.bb146.i ]
  %prefix.1.i = phi ptr [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %fmt_o.exit.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %land.lhs.true160.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %if.then168.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %fmt_x.exit.i ], [ %add.ptr154.i, %if.then153.i ], [ %prefix.0.i, %for.end12.i.i ], [ %prefix.0.i, %if.then.i.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %sw.bb146.i ]
  %pl.1.i = phi i32 [ 0, %fmt_o.exit.i ], [ 0, %land.lhs.true160.i ], [ 0, %if.then168.i ], [ 0, %fmt_x.exit.i ], [ 2, %if.then153.i ], [ %pl.0.i, %for.end12.i.i ], [ %pl.0.i, %if.then.i.i ], [ 0, %sw.bb146.i ]
  %cmp198.i = icmp slt i32 %p.2.i, 0
  %or.cond.i = select i1 %xp.0.i, i1 %cmp198.i, i1 false
  br i1 %or.cond.i, label %printf_core.exit, label %if.end201.i

if.end201.i:                                      ; preds = %ifmt_tail.i
  %and204.i = and i32 %fl.4.i, -65537
  %spec.select.i = select i1 %xp.0.i, i32 %and204.i, i32 %fl.4.i
  %tobool206.i = icmp ne i64 %arg.sroa.0.4.i, 0
  %tobool208.i = icmp ne i32 %p.2.i, 0
  %or.cond332.i = select i1 %tobool206.i, i1 true, i1 %tobool208.i
  br i1 %or.cond332.i, label %if.end210.i, label %sw.epilog280.i

if.end210.i:                                      ; preds = %if.end201.i
  %sub.ptr.rhs.cast212.i = ptrtoint ptr %a.0.i to i64
  %sub.ptr.sub213.i = sub i64 %sub.ptr.lhs.cast162.i, %sub.ptr.rhs.cast212.i
  %conv214.i = trunc i64 %sub.ptr.sub213.i to i32
  %lnot.i = xor i1 %tobool206.i, true
  %lnot.ext.i = zext i1 %lnot.i to i32
  %add216.i = add nsw i32 %conv214.i, %lnot.ext.i
  %p.2.add216.i = call i32 @llvm.smax.i32(i32 %p.2.i, i32 %add216.i)
  br label %sw.epilog280.i

sw.bb231.i:                                       ; preds = %if.end113.i
  %conv232.i = trunc i64 %arg.sroa.0.2.i to i8
  store i8 %conv232.i, ptr %add.ptr233.i, align 1, !tbaa !4
  br label %sw.epilog280.i

sw.bb235.i:                                       ; preds = %if.end113.i
  %43 = inttoptr i64 %arg.sroa.0.2.i to ptr
  %tobool236.not.i = icmp eq i64 %arg.sroa.0.2.i, 0
  %cond240.i = select i1 %tobool236.not.i, ptr @"??_C@_06OJHGLDPL@?$CInull?$CJ?$AA@", ptr %43
  %cmp241.i = icmp slt i32 %p.0.i, 0
  %narrow.i = call i32 @llvm.umin.i32(i32 %p.0.i, i32 2147483647)
  %cond247.i = zext nneg i32 %narrow.i to i64
  %call248.i = call i64 @strnlen(ptr noundef nonnull %cond240.i, i64 noundef %cond247.i) #12
  %add.ptr249.i = getelementptr inbounds i8, ptr %cond240.i, i64 %call248.i
  br i1 %cmp241.i, label %land.lhs.true252.i, label %if.end256.i

land.lhs.true252.i:                               ; preds = %sw.bb235.i
  %44 = load i8, ptr %add.ptr249.i, align 1, !tbaa !4
  %tobool254.not.i = icmp eq i8 %44, 0
  br i1 %tobool254.not.i, label %if.end256.i, label %printf_core.exit

if.end256.i:                                      ; preds = %land.lhs.true252.i, %sw.bb235.i
  %conv260.i = trunc i64 %call248.i to i32
  %.pre.i = ptrtoint ptr %add.ptr249.i to i64
  br label %sw.epilog280.i

sw.bb262.i:                                       ; preds = %if.end113.i, %if.end113.i, %if.end113.i, %if.end113.i, %if.end113.i, %if.end113.i, %if.end113.i, %if.end113.i
  %cmp265.i = icmp slt i32 %p.0.i, 0
  %or.cond333.i = select i1 %xp.0.i, i1 %cmp265.i, i1 false
  br i1 %or.cond333.i, label %printf_core.exit, label %if.end268.i

if.end268.i:                                      ; preds = %sw.bb262.i
  %45 = and i32 %t.0.i, -33
  %cmp270.i = icmp eq i32 %45, 65
  %46 = bitcast i64 %arg.sroa.0.2.i to double
  br i1 %cmp270.i, label %cond.true272.i, label %cond.false274.i

cond.true272.i:                                   ; preds = %if.end268.i
  call void @llvm.lifetime.start.p0(i64 15, ptr nonnull %buf.i.i) #13
  call void @llvm.lifetime.start.p0(i64 16, ptr nonnull %ebuf.i.i) #13
  %tobool.not.i510.i = icmp sgt i64 %arg.sroa.0.2.i, -1
  br i1 %tobool.not.i510.i, label %if.else.i521.i, label %if.then.i511.i

if.then.i511.i:                                   ; preds = %cond.true272.i
  %fneg.i.i = fneg double %46
  br label %if.end11.i512.i

if.else.i521.i:                                   ; preds = %cond.true272.i
  %and.i522.i = and i32 %fl.2.i, 2048
  %tobool1.not.i523.i = icmp eq i32 %and.i522.i, 0
  br i1 %tobool1.not.i523.i, label %if.else4.i.i, label %if.end11.i512.i

if.else4.i.i:                                     ; preds = %if.else.i521.i
  %and5.i.i = and i32 %fl.2.i, 1
  %tobool6.not.i.i = icmp eq i32 %and5.i.i, 0
  %..i.i = or disjoint i32 %and5.i.i, 2
  %.312.i.i = select i1 %tobool6.not.i.i, ptr getelementptr inbounds ([19 x i8], ptr @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", i64 0, i64 1), ptr getelementptr inbounds ([19 x i8], ptr @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", i64 0, i64 6)
  br label %if.end11.i512.i

if.end11.i512.i:                                  ; preds = %if.else4.i.i, %if.else.i521.i, %if.then.i511.i
  %pl.0.i.i = phi i32 [ 3, %if.then.i511.i ], [ 3, %if.else.i521.i ], [ %..i.i, %if.else4.i.i ]
  %prefix.0.i.i = phi ptr [ @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", %if.then.i511.i ], [ getelementptr inbounds ([19 x i8], ptr @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", i64 0, i64 3), %if.else.i521.i ], [ %.312.i.i, %if.else4.i.i ]
  %y.addr.0.i.i = phi double [ %fneg.i.i, %if.then.i511.i ], [ %46, %if.else.i521.i ], [ %46, %if.else4.i.i ]
  %and12.i.i = and i32 %t.0.i, 32
  %tobool13.not.i.i = icmp eq i32 %and12.i.i, 0
  %spec.select.idx.i.i = select i1 %tobool13.not.i.i, i64 0, i64 9
  %spec.select.i513.i = getelementptr inbounds i8, ptr %prefix.0.i.i, i64 %spec.select.idx.i.i
  %cmp.i514.i = fcmp uno double %y.addr.0.i.i, 0.000000e+00
  br i1 %cmp.i514.i, label %if.then20.i.i, label %lor.lhs.false.i515.i

lor.lhs.false.i515.i:                             ; preds = %if.end11.i512.i
  %add17.i.i = fadd double %y.addr.0.i.i, %y.addr.0.i.i
  %cmp18.i.i = fcmp oeq double %add17.i.i, %y.addr.0.i.i
  %cmp19.i.i = fcmp une double %y.addr.0.i.i, 0.000000e+00
  %or.cond.i.i = and i1 %cmp19.i.i, %cmp18.i.i
  br i1 %or.cond.i.i, label %if.then20.i.i, label %if.end37.i.i

if.then20.i.i:                                    ; preds = %lor.lhs.false.i515.i, %if.end11.i512.i
  %cond.i519.i = select i1 %tobool13.not.i.i, ptr @"??_C@_03OEIAHPBN@INF?$AA@", ptr @"??_C@_03OJHLLCGB@inf?$AA@"
  %cond27.i.i = select i1 %tobool13.not.i.i, ptr @"??_C@_03LKNCIKJB@NAN?$AA@", ptr @"??_C@_03LHCJEHON@nan?$AA@"
  %str.0.i.i = select i1 %cmp.i514.i, ptr %cond27.i.i, ptr %cond.i519.i
  %add30.i.i = add nuw nsw i32 %pl.0.i.i, 3
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add30.i.i, i32 noundef %and128.i) #14
  %conv.i520.i = zext nneg i32 %pl.0.i.i to i64
  %47 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i.i.i = icmp eq ptr %47, null
  br i1 %tobool1.not.i.i.i, label %if.else.i.i.i, label %if.then2.i.i.i

if.then2.i.i.i:                                   ; preds = %if.then20.i.i
  %48 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %49 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i.i.i = icmp ult i64 %48, %49
  br i1 %cmp.i.i.i, label %if.then3.i.i.i, label %emit.exit.i.i

if.then3.i.i.i:                                   ; preds = %if.then2.i.i.i
  %50 = xor i64 %48, -1
  %sub6.i.i.i = add i64 %49, %50
  %cond.i.i.i = call i64 @llvm.umin.i64(i64 %sub6.i.i.i, i64 %conv.i520.i)
  %add.ptr.i.i.i = getelementptr inbounds i8, ptr %47, i64 %48
  %call.i.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i.i.i, ptr noundef %spec.select.i513.i, i64 noundef %cond.i.i.i) #12
  br label %emit.exit.i.i

if.else.i.i.i:                                    ; preds = %if.then20.i.i
  call void @__msil_write(ptr noundef %spec.select.i513.i, i64 noundef %conv.i520.i) #12
  br label %emit.exit.i.i

emit.exit.i.i:                                    ; preds = %if.else.i.i.i, %if.then3.i.i.i, %if.then2.i.i.i
  %51 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i.i.i = add i64 %51, %conv.i520.i
  store i64 %add.i.i.i, ptr %len.i.i, align 8, !tbaa !38
  %52 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i314.i.i = icmp eq ptr %52, null
  br i1 %tobool1.not.i314.i.i, label %if.else.i326.i.i, label %if.then2.i315.i.i

if.then2.i315.i.i:                                ; preds = %emit.exit.i.i
  %53 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i318.i.i = icmp ult i64 %add.i.i.i, %53
  br i1 %cmp.i318.i.i, label %if.then3.i321.i.i, label %fmt_hexfloat.exit.sink.split.i

if.then3.i321.i.i:                                ; preds = %if.then2.i315.i.i
  %54 = xor i64 %add.i.i.i, -1
  %sub6.i322.i.i = add i64 %53, %54
  %cond.i323.i.i = call i64 @llvm.umin.i64(i64 %sub6.i322.i.i, i64 3)
  %add.ptr.i324.i.i = getelementptr inbounds i8, ptr %52, i64 %add.i.i.i
  %call.i325.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i324.i.i, ptr noundef nonnull %str.0.i.i, i64 noundef %cond.i323.i.i) #12
  br label %fmt_hexfloat.exit.sink.split.i

if.else.i326.i.i:                                 ; preds = %emit.exit.i.i
  call void @__msil_write(ptr noundef nonnull %str.0.i.i, i64 noundef 3) #12
  br label %fmt_hexfloat.exit.sink.split.i

if.end37.i.i:                                     ; preds = %lor.lhs.false.i515.i
  br i1 %cmp19.i.i, label %if.then40.i.i, label %if.end57.i.i

if.then40.i.i:                                    ; preds = %if.end37.i.i
  %55 = bitcast double %y.addr.0.i.i to i64
  %and42.i.i = and i64 %55, 4503599627370495
  %shr43.i.i = lshr i64 %55, 52
  %56 = trunc i64 %shr43.i.i to i32
  %conv45.i.i = and i32 %56, 2047
  %tobool46.not.i.i = icmp eq i32 %conv45.i.i, 0
  br i1 %tobool46.not.i.i, label %while.cond.i.i, label %if.then47.i.i

if.then47.i.i:                                    ; preds = %if.then40.i.i
  %sub.i518.i = add nsw i32 %conv45.i.i, -1023
  br label %if.end55.i.i

while.cond.i.i:                                   ; preds = %while.cond.i.i, %if.then40.i.i
  %k.0.i.i = phi i32 [ %dec.i.i, %while.cond.i.i ], [ 51, %if.then40.i.i ]
  %sh_prom.i.i = zext nneg i32 %k.0.i.i to i64
  %shr49.i.i = lshr i64 %and42.i.i, %sh_prom.i.i
  %tobool50.not.i.i = icmp eq i64 %shr49.i.i, 0
  %dec.i.i = add nsw i32 %k.0.i.i, -1
  br i1 %tobool50.not.i.i, label %while.cond.i.i, label %while.end.i.i, !llvm.loop !52

while.end.i.i:                                    ; preds = %while.cond.i.i
  %sub51.i.i = add nsw i32 %k.0.i.i, -1074
  %sub52.i.i = sub nsw i32 52, %k.0.i.i
  %sh_prom53.i.i = zext nneg i32 %sub52.i.i to i64
  %shl.i.i = shl i64 %55, %sh_prom53.i.i
  %and54.i.i = and i64 %shl.i.i, 4503599627370495
  br label %if.end55.i.i

if.end55.i.i:                                     ; preds = %while.end.i.i, %if.then47.i.i
  %e2.0.i.i = phi i32 [ %sub.i518.i, %if.then47.i.i ], [ %sub51.i.i, %while.end.i.i ]
  %m.0.i.i = phi i64 [ %and42.i.i, %if.then47.i.i ], [ %and54.i.i, %while.end.i.i ]
  %or.i.i = or disjoint i64 %m.0.i.i, 4607182418800017408
  %57 = bitcast i64 %or.i.i to double
  br label %if.end57.i.i

if.end57.i.i:                                     ; preds = %if.end55.i.i, %if.end37.i.i
  %e2.1.i.i = phi i32 [ %e2.0.i.i, %if.end55.i.i ], [ 0, %if.end37.i.i ]
  %y.addr.1.i.i = phi double [ %57, %if.end55.i.i ], [ %y.addr.0.i.i, %if.end37.i.i ]
  %or.cond199.i.i = icmp ult i32 %p.0.i, 13
  br i1 %or.cond199.i.i, label %if.then63.i.i, label %if.end80.i.i

if.then63.i.i:                                    ; preds = %if.end57.i.i
  %mul.i517.i = shl nuw nsw i32 %p.0.i, 2
  %sub64.i.i = sub nuw nsw i32 1075, %mul.i517.i
  %conv65.i.i = zext nneg i32 %sub64.i.i to i64
  %shl66.i.i = shl nuw nsw i64 %conv65.i.i, 52
  %58 = bitcast i64 %shl66.i.i to double
  %59 = load i8, ptr %spec.select.i513.i, align 1, !tbaa !4
  %cmp69.i.i = icmp eq i8 %59, 45
  br i1 %cmp69.i.i, label %if.then71.i.i, label %if.else76.i.i

if.then71.i.i:                                    ; preds = %if.then63.i.i
  %fneg72.i.i = fneg double %y.addr.1.i.i
  %sub73.i.i = fsub double %fneg72.i.i, %58
  %add74.i.i = fadd double %sub73.i.i, %58
  %fneg75.i.i = fneg double %add74.i.i
  br label %if.end80.i.i

if.else76.i.i:                                    ; preds = %if.then63.i.i
  %add77.i.i = fadd double %y.addr.1.i.i, %58
  %sub78.i.i = fsub double %add77.i.i, %58
  br label %if.end80.i.i

if.end80.i.i:                                     ; preds = %if.else76.i.i, %if.then71.i.i, %if.end57.i.i
  %y.addr.3.i.i = phi double [ %y.addr.1.i.i, %if.end57.i.i ], [ %fneg75.i.i, %if.then71.i.i ], [ %sub78.i.i, %if.else76.i.i ]
  %cmp81.i.i = icmp slt i32 %e2.1.i.i, 0
  %cond89.v.i.i = call i32 @llvm.abs.i32(i32 %e2.1.i.i, i1 true)
  %cmp330.i.i.i = icmp ugt i32 %cond89.v.i.i, 9
  br i1 %cmp330.i.i.i, label %for.body5.i.i.i, label %for.end12.i.i.i

for.body5.i.i.i:                                  ; preds = %for.body5.i.i.i, %if.end80.i.i
  %y.032.i.i.i = phi i32 [ %div11.i.i.i, %for.body5.i.i.i ], [ %cond89.v.i.i, %if.end80.i.i ]
  %s.addr.131.i.idx.i.i = phi i64 [ %s.addr.131.i.add.i.i, %for.body5.i.i.i ], [ 16, %if.end80.i.i ]
  %rem6.i.i.i = urem i32 %y.032.i.i.i, 10
  %60 = trunc i32 %rem6.i.i.i to i8
  %conv8.i.i.i = or disjoint i8 %60, 48
  %s.addr.131.i.add.i.i = add nsw i64 %s.addr.131.i.idx.i.i, -1
  %incdec.ptr9.i.ptr.i.i = getelementptr inbounds i8, ptr %ebuf.i.i, i64 %s.addr.131.i.add.i.i
  store i8 %conv8.i.i.i, ptr %incdec.ptr9.i.ptr.i.i, align 1, !tbaa !4
  %div11.i.i.i = udiv i32 %y.032.i.i.i, 10
  %cmp3.i.i.i = icmp ugt i32 %y.032.i.i.i, 99
  br i1 %cmp3.i.i.i, label %for.body5.i.i.i, label %for.end12.i.i.i, !llvm.loop !51

for.end12.i.i.i:                                  ; preds = %for.body5.i.i.i, %if.end80.i.i
  %s.addr.1.lcssa.i.idx.i.i = phi i64 [ 16, %if.end80.i.i ], [ %s.addr.131.i.add.i.i, %for.body5.i.i.i ]
  %y.0.lcssa.i.i.i = phi i32 [ %cond89.v.i.i, %if.end80.i.i ], [ %div11.i.i.i, %for.body5.i.i.i ]
  %tobool.not.i.i.i = icmp eq i32 %y.0.lcssa.i.i.i, 0
  br i1 %tobool.not.i.i.i, label %fmt_u.exit.i.i, label %if.then.i.i.i

if.then.i.i.i:                                    ; preds = %for.end12.i.i.i
  %61 = trunc i32 %y.0.lcssa.i.i.i to i8
  %conv14.i.i.i = or disjoint i8 %61, 48
  %s.addr.1.lcssa.i.add.i.i = add nsw i64 %s.addr.1.lcssa.i.idx.i.i, -1
  %incdec.ptr15.i.ptr.i.i = getelementptr inbounds i8, ptr %ebuf.i.i, i64 %s.addr.1.lcssa.i.add.i.i
  store i8 %conv14.i.i.i, ptr %incdec.ptr15.i.ptr.i.i, align 1, !tbaa !4
  br label %fmt_u.exit.i.i

fmt_u.exit.i.i:                                   ; preds = %if.then.i.i.i, %for.end12.i.i.i
  %s.addr.2.i.idx.i.i = phi i64 [ %s.addr.1.lcssa.i.add.i.i, %if.then.i.i.i ], [ %s.addr.1.lcssa.i.idx.i.i, %for.end12.i.i.i ]
  %cmp91.i.i = icmp eq i64 %s.addr.2.i.idx.i.i, 16
  br i1 %cmp91.i.i, label %if.then93.i.i, label %if.end95.i.i

if.then93.i.i:                                    ; preds = %fmt_u.exit.i.i
  store i8 48, ptr %incdec.ptr94.ptr.i.i, align 1, !tbaa !4
  br label %if.end95.i.i

if.end95.i.i:                                     ; preds = %if.then93.i.i, %fmt_u.exit.i.i
  %estr.0.idx.i.i = phi i64 [ 15, %if.then93.i.i ], [ %s.addr.2.i.idx.i.i, %fmt_u.exit.i.i ]
  %conv99.i.i = select i1 %cmp81.i.i, i8 45, i8 43
  %gep.i = getelementptr i8, ptr %invariant.gep.i, i64 %estr.0.idx.i.i
  store i8 %conv99.i.i, ptr %gep.i, align 1, !tbaa !4
  %62 = trunc i32 %t.0.i to i8
  %conv102.i.i = add nuw nsw i8 %62, 15
  %estr.0.add.i.i = add nsw i64 %estr.0.idx.i.i, -2
  %incdec.ptr103.ptr.i.i = getelementptr inbounds i8, ptr %ebuf.i.i, i64 %estr.0.add.i.i
  store i8 %conv102.i.i, ptr %incdec.ptr103.ptr.i.i, align 1, !tbaa !4
  %63 = trunc i32 %and12.i.i to i8
  %cmp120.i.i = icmp slt i32 %p.0.i, 1
  %and123.i.i = and i32 %fl.2.i, 8
  %tobool124.not.i.i = icmp eq i32 %and123.i.i, 0
  br label %do.body.i.i

do.body.i.i:                                      ; preds = %if.end127.i.i, %if.end95.i.i
  %s.0.i.i = phi ptr [ %buf.i.i, %if.end95.i.i ], [ %s.1.i.i, %if.end127.i.i ]
  %y.addr.4.i.i = phi double [ %y.addr.3.i.i, %if.end95.i.i ], [ %mul113.i.i, %if.end127.i.i ]
  %conv105.i.i = fptosi double %y.addr.4.i.i to i32
  %idxprom.i.i = sext i32 %conv105.i.i to i64
  %arrayidx.i516.i = getelementptr inbounds [16 x i8], ptr @xdigits, i64 0, i64 %idxprom.i.i
  %64 = load i8, ptr %arrayidx.i516.i, align 1, !tbaa !4
  %conv109.i.i = or i8 %64, %63
  %incdec.ptr110.i.i = getelementptr inbounds i8, ptr %s.0.i.i, i64 1
  store i8 %conv109.i.i, ptr %s.0.i.i, align 1, !tbaa !4
  %conv111.i.i = sitofp i32 %conv105.i.i to double
  %sub112.i.i = fsub double %y.addr.4.i.i, %conv111.i.i
  %mul113.i.i = fmul double %sub112.i.i, 1.600000e+01
  %sub.ptr.lhs.cast.i.i = ptrtoint ptr %incdec.ptr110.i.i to i64
  %sub.ptr.sub.i.i = sub i64 %sub.ptr.lhs.cast.i.i, %sub.ptr.rhs.cast.i.i
  %cmp115.i.i = icmp eq i64 %sub.ptr.sub.i.i, 1
  br i1 %cmp115.i.i, label %land.lhs.true117.i.i, label %if.end127.i.i

land.lhs.true117.i.i:                             ; preds = %do.body.i.i
  %tobool118.i.i = fcmp oeq double %mul113.i.i, 0.000000e+00
  %or.cond200.not374.i.i = and i1 %cmp120.i.i, %tobool118.i.i
  %or.cond313.i.i = and i1 %tobool124.not.i.i, %or.cond200.not374.i.i
  br i1 %or.cond313.i.i, label %if.end127.i.i, label %if.then125.i.i

if.then125.i.i:                                   ; preds = %land.lhs.true117.i.i
  %incdec.ptr126.i.i = getelementptr inbounds i8, ptr %s.0.i.i, i64 2
  store i8 46, ptr %incdec.ptr110.i.i, align 1, !tbaa !4
  br label %if.end127.i.i

if.end127.i.i:                                    ; preds = %if.then125.i.i, %land.lhs.true117.i.i, %do.body.i.i
  %s.1.i.i = phi ptr [ %incdec.ptr126.i.i, %if.then125.i.i ], [ %incdec.ptr110.i.i, %do.body.i.i ], [ %incdec.ptr110.i.i, %land.lhs.true117.i.i ]
  %tobool128.i.i = fcmp une double %mul113.i.i, 0.000000e+00
  br i1 %tobool128.i.i, label %do.body.i.i, label %do.end.i.i, !llvm.loop !53

do.end.i.i:                                       ; preds = %if.end127.i.i
  %tobool129.not.i.i = icmp eq i32 %p.0.i, 0
  %.pre.i.i = ptrtoint ptr %s.1.i.i to i64
  %.pre377.i.i = sub i64 %.pre.i.i, %sub.ptr.rhs.cast.i.i
  br i1 %tobool129.not.i.i, label %if.else146.i.i, label %land.lhs.true130.i.i

land.lhs.true130.i.i:                             ; preds = %do.end.i.i
  %sub135.i.i = add i64 %.pre377.i.i, -2
  %conv136.i.i = sext i32 %p.0.i to i64
  %cmp137.i.i = icmp slt i64 %sub135.i.i, %conv136.i.i
  br i1 %cmp137.i.i, label %if.end157.i.i, label %if.else146.i.i

if.else146.i.i:                                   ; preds = %land.lhs.true130.i.i, %do.end.i.i
  %conv151.i.i = trunc i64 %.pre377.i.i to i32
  br label %if.end157.i.i

if.end157.i.i:                                    ; preds = %if.else146.i.i, %land.lhs.true130.i.i
  %conv151.sink.i.i = phi i32 [ %conv151.i.i, %if.else146.i.i ], [ %p.0.i, %land.lhs.true130.i.i ]
  %.sink.i.i = phi i32 [ 18, %if.else146.i.i ], [ 20, %land.lhs.true130.i.i ]
  %65 = trunc i64 %estr.0.idx.i.i to i32
  %reass.sub375.i.i = sub i32 %conv151.sink.i.i, %65
  %add156.i.i = add i32 %reass.sub375.i.i, %.sink.i.i
  %add158.i.i = add nsw i32 %add156.i.i, %pl.0.i.i
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add158.i.i, i32 noundef %fl.2.i) #14
  %conv159.i.i = zext nneg i32 %pl.0.i.i to i64
  %66 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i328.i.i = icmp eq ptr %66, null
  br i1 %tobool1.not.i328.i.i, label %if.else.i340.i.i, label %if.then2.i329.i.i

if.then2.i329.i.i:                                ; preds = %if.end157.i.i
  %67 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %68 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i332.i.i = icmp ult i64 %67, %68
  br i1 %cmp.i332.i.i, label %if.then3.i335.i.i, label %emit.exit341.i.i

if.then3.i335.i.i:                                ; preds = %if.then2.i329.i.i
  %69 = xor i64 %67, -1
  %sub6.i336.i.i = add i64 %68, %69
  %cond.i337.i.i = call i64 @llvm.umin.i64(i64 %sub6.i336.i.i, i64 %conv159.i.i)
  %add.ptr.i338.i.i = getelementptr inbounds i8, ptr %66, i64 %67
  %call.i339.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i338.i.i, ptr noundef %spec.select.i513.i, i64 noundef %cond.i337.i.i) #12
  br label %emit.exit341.i.i

if.else.i340.i.i:                                 ; preds = %if.end157.i.i
  call void @__msil_write(ptr noundef %spec.select.i513.i, i64 noundef %conv159.i.i) #12
  br label %emit.exit341.i.i

emit.exit341.i.i:                                 ; preds = %if.else.i340.i.i, %if.then3.i335.i.i, %if.then2.i329.i.i
  %70 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i334.i.i = add i64 %70, %conv159.i.i
  store i64 %add.i334.i.i, ptr %len.i.i, align 8, !tbaa !38
  %xor160.i.i = xor i32 %fl.2.i, 65536
  call fastcc void @pad(ptr noundef nonnull %o, i8 noundef 48, i32 noundef %w.0.i, i32 noundef %add158.i.i, i32 noundef %xor160.i.i) #14
  %tobool.not.i342.i.i = icmp eq ptr %s.1.i.i, %buf.i.i
  br i1 %tobool.not.i342.i.i, label %emit.exit356.i.i, label %if.end.i.i.i

if.end.i.i.i:                                     ; preds = %emit.exit341.i.i
  %71 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i343.i.i = icmp eq ptr %71, null
  br i1 %tobool1.not.i343.i.i, label %if.else.i355.i.i, label %if.then2.i344.i.i

if.then2.i344.i.i:                                ; preds = %if.end.i.i.i
  %72 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %73 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i347.i.i = icmp ult i64 %72, %73
  br i1 %cmp.i347.i.i, label %if.then3.i350.i.i, label %if.end11.i.i.i

if.then3.i350.i.i:                                ; preds = %if.then2.i344.i.i
  %74 = xor i64 %72, -1
  %sub6.i351.i.i = add i64 %73, %74
  %cond.i352.i.i = call i64 @llvm.umin.i64(i64 %sub6.i351.i.i, i64 %.pre377.i.i)
  %add.ptr.i353.i.i = getelementptr inbounds i8, ptr %71, i64 %72
  %call.i354.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i353.i.i, ptr noundef nonnull %buf.i.i, i64 noundef %cond.i352.i.i) #12
  br label %if.end11.i.i.i

if.else.i355.i.i:                                 ; preds = %if.end.i.i.i
  call void @__msil_write(ptr noundef nonnull %buf.i.i, i64 noundef %.pre377.i.i) #12
  br label %if.end11.i.i.i

if.end11.i.i.i:                                   ; preds = %if.else.i355.i.i, %if.then3.i350.i.i, %if.then2.i344.i.i
  %75 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i349.i.i = add i64 %75, %.pre377.i.i
  store i64 %add.i349.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %emit.exit356.i.i

emit.exit356.i.i:                                 ; preds = %if.end11.i.i.i, %emit.exit341.i.i
  %gepdiff.i.i = sub nsw i64 18, %estr.0.idx.i.i
  %conv170.neg376.i.i = add i32 %65, -18
  %conv176.i.i = trunc i64 %.pre377.i.i to i32
  %.neg.i.i = sub i32 %conv170.neg376.i.i, %conv176.i.i
  %sub177.i.i = add i32 %.neg.i.i, %add156.i.i
  call fastcc void @pad(ptr noundef nonnull %o, i8 noundef 48, i32 noundef %sub177.i.i, i32 noundef 0, i32 noundef 0) #14
  %tobool.not.i357.i.i = icmp eq i64 %estr.0.add.i.i, 16
  br i1 %tobool.not.i357.i.i, label %fmt_hexfloat.exit.i, label %if.end.i358.i.i

if.end.i358.i.i:                                  ; preds = %emit.exit356.i.i
  %76 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i359.i.i = icmp eq ptr %76, null
  br i1 %tobool1.not.i359.i.i, label %if.else.i372.i.i, label %if.then2.i360.i.i

if.then2.i360.i.i:                                ; preds = %if.end.i358.i.i
  %77 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %78 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i363.i.i = icmp ult i64 %77, %78
  br i1 %cmp.i363.i.i, label %if.then3.i367.i.i, label %fmt_hexfloat.exit.sink.split.i

if.then3.i367.i.i:                                ; preds = %if.then2.i360.i.i
  %79 = xor i64 %77, -1
  %sub6.i368.i.i = add i64 %78, %79
  %cond.i369.i.i = call i64 @llvm.umin.i64(i64 %sub6.i368.i.i, i64 %gepdiff.i.i)
  %add.ptr.i370.i.i = getelementptr inbounds i8, ptr %76, i64 %77
  %call.i371.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i370.i.i, ptr noundef nonnull %incdec.ptr103.ptr.i.i, i64 noundef %cond.i369.i.i) #12
  br label %fmt_hexfloat.exit.sink.split.i

if.else.i372.i.i:                                 ; preds = %if.end.i358.i.i
  call void @__msil_write(ptr noundef nonnull %incdec.ptr103.ptr.i.i, i64 noundef %gepdiff.i.i) #12
  br label %fmt_hexfloat.exit.sink.split.i

fmt_hexfloat.exit.sink.split.i:                   ; preds = %if.else.i372.i.i, %if.then3.i367.i.i, %if.then2.i360.i.i, %if.else.i326.i.i, %if.then3.i321.i.i, %if.then2.i315.i.i
  %gepdiff.i.sink.i = phi i64 [ 3, %if.else.i326.i.i ], [ 3, %if.then3.i321.i.i ], [ 3, %if.then2.i315.i.i ], [ %gepdiff.i.i, %if.else.i372.i.i ], [ %gepdiff.i.i, %if.then3.i367.i.i ], [ %gepdiff.i.i, %if.then2.i360.i.i ]
  %add30.i.sink714.ph.i = phi i32 [ %add30.i.i, %if.else.i326.i.i ], [ %add30.i.i, %if.then3.i321.i.i ], [ %add30.i.i, %if.then2.i315.i.i ], [ %add158.i.i, %if.else.i372.i.i ], [ %add158.i.i, %if.then3.i367.i.i ], [ %add158.i.i, %if.then2.i360.i.i ]
  %80 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i366.i.i = add i64 %80, %gepdiff.i.sink.i
  store i64 %add.i366.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %fmt_hexfloat.exit.i

fmt_hexfloat.exit.i:                              ; preds = %fmt_hexfloat.exit.sink.split.i, %emit.exit356.i.i
  %add30.i.sink714.i = phi i32 [ %add158.i.i, %emit.exit356.i.i ], [ %add30.i.sink714.ph.i, %fmt_hexfloat.exit.sink.split.i ]
  %xor.i.i = xor i32 %fl.2.i, 8192
  call fastcc void @pad(ptr noundef nonnull %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add30.i.sink714.i, i32 noundef %xor.i.i) #14
  %cond36.i.i = call i32 @llvm.smax.i32(i32 %add30.i.sink714.i, i32 %w.0.i)
  call void @llvm.lifetime.end.p0(i64 16, ptr nonnull %ebuf.i.i) #13
  call void @llvm.lifetime.end.p0(i64 15, ptr nonnull %buf.i.i) #13
  br label %cond.end276.i

cond.false274.i:                                  ; preds = %if.end268.i
  call void @llvm.lifetime.start.p0(i64 576, ptr nonnull %big.i.i) #13
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %buf.i524.i) #13
  call void @llvm.lifetime.start.p0(i64 16, ptr nonnull %ebuf.i525.i) #13
  %tobool.not.i527.i = icmp sgt i64 %arg.sroa.0.2.i, -1
  br i1 %tobool.not.i527.i, label %if.else.i586.i, label %if.then.i528.i

if.then.i528.i:                                   ; preds = %cond.false274.i
  %fneg.i529.i = fneg double %46
  br label %if.end11.i530.i

if.else.i586.i:                                   ; preds = %cond.false274.i
  %and.i587.i = and i32 %fl.2.i, 2048
  %tobool1.not.i588.i = icmp eq i32 %and.i587.i, 0
  br i1 %tobool1.not.i588.i, label %if.else4.i589.i, label %if.end11.i530.i

if.else4.i589.i:                                  ; preds = %if.else.i586.i
  %and5.i590.i = and i32 %fl.2.i, 1
  %tobool6.not.not.i.i = icmp eq i32 %and5.i590.i, 0
  %..i591.i = select i1 %tobool6.not.not.i.i, ptr getelementptr inbounds ([19 x i8], ptr @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", i64 0, i64 1), ptr getelementptr inbounds ([19 x i8], ptr @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", i64 0, i64 6)
  br label %if.end11.i530.i

if.end11.i530.i:                                  ; preds = %if.else4.i589.i, %if.else.i586.i, %if.then.i528.i
  %y.addr.0.i531.i = phi double [ %fneg.i529.i, %if.then.i528.i ], [ %46, %if.else.i586.i ], [ %46, %if.else4.i589.i ]
  %prefix.0.i532.i = phi ptr [ @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", %if.then.i528.i ], [ getelementptr inbounds ([19 x i8], ptr @"??_C@_0BD@JDHLFIKM@?90X?$CL0X?50X?90x?$CL0x?50x?$AA@", i64 0, i64 3), %if.else.i586.i ], [ %..i591.i, %if.else4.i589.i ]
  %pl.0.i533.i = phi i32 [ 1, %if.then.i528.i ], [ 1, %if.else.i586.i ], [ %and5.i590.i, %if.else4.i589.i ]
  %cmp.i534.i = fcmp uno double %y.addr.0.i531.i, 0.000000e+00
  br i1 %cmp.i534.i, label %if.then14.i.i, label %lor.lhs.false.i535.i

lor.lhs.false.i535.i:                             ; preds = %if.end11.i530.i
  %add.i536.i = fadd double %y.addr.0.i531.i, %y.addr.0.i531.i
  %cmp12.i.i = fcmp oeq double %add.i536.i, %y.addr.0.i531.i
  %cmp13.i.i = fcmp une double %y.addr.0.i531.i, 0.000000e+00
  %or.cond.i537.i = and i1 %cmp13.i.i, %cmp12.i.i
  br i1 %or.cond.i537.i, label %if.then14.i.i, label %if.end31.i.i

if.then14.i.i:                                    ; preds = %lor.lhs.false.i535.i, %if.end11.i530.i
  %and15.i.i = and i32 %t.0.i, 32
  %tobool16.not.i.i = icmp eq i32 %and15.i.i, 0
  %cond.i565.i = select i1 %tobool16.not.i.i, ptr @"??_C@_03OEIAHPBN@INF?$AA@", ptr @"??_C@_03OJHLLCGB@inf?$AA@"
  %cond21.i.i = select i1 %tobool16.not.i.i, ptr @"??_C@_03LKNCIKJB@NAN?$AA@", ptr @"??_C@_03LHCJEHON@nan?$AA@"
  %str.0.i566.i = select i1 %cmp.i534.i, ptr %cond21.i.i, ptr %cond.i565.i
  %add24.i.i = add nuw nsw i32 %pl.0.i533.i, 3
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add24.i.i, i32 noundef %and128.i) #14
  %conv.i567.i = zext nneg i32 %pl.0.i533.i to i64
  %tobool.not.i.i568.i = icmp eq i32 %pl.0.i533.i, 0
  %.pre1164.i.i = load ptr, ptr %o, align 8, !tbaa !35
  br i1 %tobool.not.i.i568.i, label %emit.exit.i578.i, label %if.end.i.i569.i

if.end.i.i569.i:                                  ; preds = %if.then14.i.i
  %tobool1.not.i.i570.i = icmp eq ptr %.pre1164.i.i, null
  br i1 %tobool1.not.i.i570.i, label %if.else.i.i585.i, label %if.then2.i.i571.i

if.then2.i.i571.i:                                ; preds = %if.end.i.i569.i
  %81 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %82 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i.i574.i = icmp ult i64 %81, %82
  br i1 %cmp.i.i574.i, label %if.then3.i.i580.i, label %if.end11.i.i575.i

if.then3.i.i580.i:                                ; preds = %if.then2.i.i571.i
  %83 = xor i64 %81, -1
  %sub6.i.i581.i = add i64 %82, %83
  %cond.i.i582.i = call i64 @llvm.umin.i64(i64 %sub6.i.i581.i, i64 %conv.i567.i)
  %add.ptr.i.i583.i = getelementptr inbounds i8, ptr %.pre1164.i.i, i64 %81
  %call.i.i584.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i.i583.i, ptr noundef %prefix.0.i532.i, i64 noundef %cond.i.i582.i) #12
  br label %if.end11.i.i575.i

if.else.i.i585.i:                                 ; preds = %if.end.i.i569.i
  call void @__msil_write(ptr noundef %prefix.0.i532.i, i64 noundef %conv.i567.i) #12
  br label %if.end11.i.i575.i

if.end11.i.i575.i:                                ; preds = %if.else.i.i585.i, %if.then3.i.i580.i, %if.then2.i.i571.i
  %84 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i.i577.i = add i64 %84, %conv.i567.i
  store i64 %add.i.i577.i, ptr %len.i.i, align 8, !tbaa !38
  %.pre1163.i.i = load ptr, ptr %o, align 8, !tbaa !35
  br label %emit.exit.i578.i

emit.exit.i578.i:                                 ; preds = %if.end11.i.i575.i, %if.then14.i.i
  %85 = phi ptr [ %.pre1164.i.i, %if.then14.i.i ], [ %.pre1163.i.i, %if.end11.i.i575.i ]
  %tobool1.not.i810.i.i = icmp eq ptr %85, null
  br i1 %tobool1.not.i810.i.i, label %if.else.i823.i.i, label %if.then2.i811.i.i

if.then2.i811.i.i:                                ; preds = %emit.exit.i578.i
  %86 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %87 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i814.i.i = icmp ult i64 %86, %87
  br i1 %cmp.i814.i.i, label %if.then3.i818.i.i, label %emit.exit824.i.i

if.then3.i818.i.i:                                ; preds = %if.then2.i811.i.i
  %88 = xor i64 %86, -1
  %sub6.i819.i.i = add i64 %87, %88
  %cond.i820.i.i = call i64 @llvm.umin.i64(i64 %sub6.i819.i.i, i64 3)
  %add.ptr.i821.i.i = getelementptr inbounds i8, ptr %85, i64 %86
  %call.i822.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i821.i.i, ptr noundef nonnull %str.0.i566.i, i64 noundef %cond.i820.i.i) #12
  br label %emit.exit824.i.i

if.else.i823.i.i:                                 ; preds = %emit.exit.i578.i
  call void @__msil_write(ptr noundef nonnull %str.0.i566.i, i64 noundef 3) #12
  br label %emit.exit824.i.i

emit.exit824.i.i:                                 ; preds = %if.else.i823.i.i, %if.then3.i818.i.i, %if.then2.i811.i.i
  %89 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i817.i.i = add i64 %89, 3
  store i64 %add.i817.i.i, ptr %len.i.i, align 8, !tbaa !38
  %xor.i579.i = xor i32 %fl.2.i, 8192
  call fastcc void @pad(ptr noundef nonnull %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add24.i.i, i32 noundef %xor.i579.i) #14
  %cond30.i.i = call i32 @llvm.smax.i32(i32 %add24.i.i, i32 %w.0.i)
  br label %fmt_fp.exit.i

if.end31.i.i:                                     ; preds = %lor.lhs.false.i535.i
  %spec.store.select.i.i = select i1 %cmp265.i, i32 6, i32 %p.0.i
  %90 = bitcast double %y.addr.0.i531.i to i64
  %shr.i.i.i = lshr i64 %90, 52
  %91 = trunc i64 %shr.i.i.i to i32
  %conv.i.i.i = and i32 %91, 2047
  %cmp.i825.i.i = fcmp oeq double %y.addr.0.i531.i, 0.000000e+00
  br i1 %cmp.i825.i.i, label %if.then39.thread.i.i, label %if.end.i826.i.i

if.then39.thread.i.i:                             ; preds = %if.end31.i.i
  store i32 0, ptr %big.i.i, align 16, !tbaa !24
  br label %if.end45.i.i

if.end.i826.i.i:                                  ; preds = %if.end31.i.i
  %cmp2.i.i.i = icmp eq i32 %conv.i.i.i, 0
  br i1 %cmp2.i.i.i, label %if.then4.i.i.i, label %if.else.i827.i.i

if.then4.i.i.i:                                   ; preds = %if.end.i826.i.i
  %mul.i.i.i = fmul double %y.addr.0.i531.i, 0x4330000000000000
  %92 = bitcast double %mul.i.i.i to i64
  %shr6.i.i.i = lshr i64 %92, 52
  %93 = trunc i64 %shr6.i.i.i to i32
  %conv8.i.i564.i = and i32 %93, 2047
  %sub9.i.i.i = add nsw i32 %conv8.i.i564.i, -1075
  br label %if.end11.i828.i.i

if.else.i827.i.i:                                 ; preds = %if.end.i826.i.i
  %sub10.i.i.i = add nsw i32 %conv.i.i.i, -1023
  br label %if.end11.i828.i.i

if.end11.i828.i.i:                                ; preds = %if.else.i827.i.i, %if.then4.i.i.i
  %e2.0.i.i.i = phi i32 [ %sub9.i.i.i, %if.then4.i.i.i ], [ %sub10.i.i.i, %if.else.i827.i.i ]
  %bits.0.i.i.i = phi i64 [ %92, %if.then4.i.i.i ], [ %90, %if.else.i827.i.i ]
  %and12.i.i.i = and i64 %bits.0.i.i.i, 4503599627370495
  %or.i.i.i = or disjoint i64 %and12.i.i.i, 4607182418800017408
  %94 = bitcast i64 %or.i.i.i to double
  %mul14.i.i.i = fmul double %94, 0x41B0000000000000
  %cmp16.i.i.i = icmp slt i32 %e2.0.i.i.i, 28
  %z.0.idx.i.i.i = select i1 %cmp16.i.i.i, i64 0, i64 80
  %z.0.i.i.i = getelementptr inbounds i32, ptr %big.i.i, i64 %z.0.idx.i.i.i
  br label %for.cond.i.i.i

for.cond.i.i.i:                                   ; preds = %for.cond.i.i.i, %if.end11.i828.i.i
  %z.1.i.i.i = phi ptr [ %z.0.i.i.i, %if.end11.i828.i.i ], [ %incdec.ptr.i.i.i, %for.cond.i.i.i ]
  %y0.0.i.i.i = phi double [ %mul14.i.i.i, %if.end11.i828.i.i ], [ %mul26.i.i.i, %for.cond.i.i.i ]
  %conv23.i.i.i = fptoui double %y0.0.i.i.i to i32
  %incdec.ptr.i.i.i = getelementptr inbounds i32, ptr %z.1.i.i.i, i64 1
  store i32 %conv23.i.i.i, ptr %z.1.i.i.i, align 4, !tbaa !24
  %conv24.i.i.i = uitofp i32 %conv23.i.i.i to double
  %sub25.i.i.i = fsub double %y0.0.i.i.i, %conv24.i.i.i
  %mul26.i.i.i = fmul double %sub25.i.i.i, 1.000000e+09
  %cmp27.i.i.i = fcmp oeq double %mul26.i.i.i, 0.000000e+00
  br i1 %cmp27.i.i.i, label %double_init.exit.i.i, label %for.cond.i.i.i

double_init.exit.i.i:                             ; preds = %for.cond.i.i.i
  %sub15.i.i.i = add nsw i32 %e2.0.i.i.i, -28
  %cmp91.i.i.i = icmp sgt i32 %e2.0.i.i.i, 28
  br i1 %cmp91.i.i.i, label %while.body.i.i.i, label %while.cond14.preheader.i.i.i

while.cond14.preheader.i.i.i:                     ; preds = %while.end.i.i.i, %double_init.exit.i.i
  %z.0.lcssa.i.i.i = phi ptr [ %incdec.ptr.i.i.i, %double_init.exit.i.i ], [ %z.1.i839.i.i, %while.end.i.i.i ]
  %a.0.lcssa.i.i.i = phi ptr [ %z.0.i.i.i, %double_init.exit.i.i ], [ %a.1.i.i.i, %while.end.i.i.i ]
  %e2.addr.0.lcssa.i.i.i = phi i32 [ %sub15.i.i.i, %double_init.exit.i.i ], [ %sub.i.i.i, %while.end.i.i.i ]
  %cmp15102.i.i.i = icmp slt i32 %e2.addr.0.lcssa.i.i.i, 0
  br i1 %cmp15102.i.i.i, label %while.body17.i.i.i, label %double_scale.exit.i.i

while.body.i.i.i:                                 ; preds = %while.end.i.i.i, %double_init.exit.i.i
  %e2.addr.094.i.i.i = phi i32 [ %sub.i.i.i, %while.end.i.i.i ], [ %sub15.i.i.i, %double_init.exit.i.i ]
  %a.093.i.i.i = phi ptr [ %a.1.i.i.i, %while.end.i.i.i ], [ %z.0.i.i.i, %double_init.exit.i.i ]
  %z.092.i.i.i = phi ptr [ %z.1.i839.i.i, %while.end.i.i.i ], [ %incdec.ptr.i.i.i, %double_init.exit.i.i ]
  %cond.i832.i.i = call i32 @llvm.smin.i32(i32 %e2.addr.094.i.i.i, i32 29)
  %d.087.i.i.i = getelementptr inbounds i32, ptr %z.092.i.i.i, i64 -1
  %cmp2.not88.i.i.i = icmp ult ptr %d.087.i.i.i, %a.093.i.i.i
  br i1 %cmp2.not88.i.i.i, label %if.end.i838.i.i, label %for.body.lr.ph.i.i.i

for.body.lr.ph.i.i.i:                             ; preds = %while.body.i.i.i
  %sh_prom.i.i.i = zext nneg i32 %cond.i832.i.i to i64
  br label %for.body.i.i.i

for.body.i.i.i:                                   ; preds = %for.body.i.i.i, %for.body.lr.ph.i.i.i
  %d.090.i.i.i = phi ptr [ %d.087.i.i.i, %for.body.lr.ph.i.i.i ], [ %d.0.i.i.i, %for.body.i.i.i ]
  %carry.089.i.i.i = phi i64 [ 0, %for.body.lr.ph.i.i.i ], [ %div.i.i.i, %for.body.i.i.i ]
  %95 = load i32, ptr %d.090.i.i.i, align 4, !tbaa !24
  %conv.i833.i.i = zext i32 %95 to i64
  %shl.i.i.i = shl nuw nsw i64 %conv.i833.i.i, %sh_prom.i.i.i
  %conv3.i.i.i = and i64 %carry.089.i.i.i, 4294967295
  %add.i834.i.i = add nuw i64 %shl.i.i.i, %conv3.i.i.i
  %rem.i.i.i = urem i64 %add.i834.i.i, 1000000000
  %conv4.i.i.i = trunc i64 %rem.i.i.i to i32
  store i32 %conv4.i.i.i, ptr %d.090.i.i.i, align 4, !tbaa !24
  %div.i.i.i = udiv i64 %add.i834.i.i, 1000000000
  %d.0.i.i.i = getelementptr inbounds i32, ptr %d.090.i.i.i, i64 -1
  %cmp2.not.i.i.i = icmp ult ptr %d.0.i.i.i, %a.093.i.i.i
  br i1 %cmp2.not.i.i.i, label %for.end.i835.i.i, label %for.body.i.i.i, !llvm.loop !54

for.end.i835.i.i:                                 ; preds = %for.body.i.i.i
  %conv5.i.i.i = trunc i64 %div.i.i.i to i32
  %tobool.not.i836.i.i = icmp eq i32 %conv5.i.i.i, 0
  br i1 %tobool.not.i836.i.i, label %if.end.i838.i.i, label %if.then.i837.i.i

if.then.i837.i.i:                                 ; preds = %for.end.i835.i.i
  %incdec.ptr6.i.i.i = getelementptr inbounds i32, ptr %a.093.i.i.i, i64 -1
  store i32 %conv5.i.i.i, ptr %incdec.ptr6.i.i.i, align 4, !tbaa !24
  br label %if.end.i838.i.i

if.end.i838.i.i:                                  ; preds = %if.then.i837.i.i, %for.end.i835.i.i, %while.body.i.i.i
  %a.1.i.i.i = phi ptr [ %incdec.ptr6.i.i.i, %if.then.i837.i.i ], [ %a.093.i.i.i, %for.end.i835.i.i ], [ %a.093.i.i.i, %while.body.i.i.i ]
  br label %while.cond7.i.i.i

while.cond7.i.i.i:                                ; preds = %land.rhs.i.i.i, %if.end.i838.i.i
  %z.1.i839.i.i = phi ptr [ %z.092.i.i.i, %if.end.i838.i.i ], [ %arrayidx.i.i.i, %land.rhs.i.i.i ]
  %cmp8.i.i.i = icmp ugt ptr %z.1.i839.i.i, %a.1.i.i.i
  br i1 %cmp8.i.i.i, label %land.rhs.i.i.i, label %while.end.i.i.i

land.rhs.i.i.i:                                   ; preds = %while.cond7.i.i.i
  %arrayidx.i.i.i = getelementptr inbounds i32, ptr %z.1.i839.i.i, i64 -1
  %96 = load i32, ptr %arrayidx.i.i.i, align 4, !tbaa !24
  %tobool10.not.i.i.i = icmp eq i32 %96, 0
  br i1 %tobool10.not.i.i.i, label %while.cond7.i.i.i, label %while.end.i.i.i, !llvm.loop !55

while.end.i.i.i:                                  ; preds = %land.rhs.i.i.i, %while.cond7.i.i.i
  %sub.i.i.i = sub nsw i32 %e2.addr.094.i.i.i, %cond.i832.i.i
  %cmp.i840.i.i = icmp sgt i32 %sub.i.i.i, 0
  br i1 %cmp.i840.i.i, label %while.body.i.i.i, label %while.cond14.preheader.i.i.i, !llvm.loop !56

while.body17.i.i.i:                               ; preds = %if.end47.i.i.i, %while.cond14.preheader.i.i.i
  %e2.addr.1105.i.i.i = phi i32 [ %add48.i.i.i, %if.end47.i.i.i ], [ %e2.addr.0.lcssa.i.i.i, %while.cond14.preheader.i.i.i ]
  %a.2104.i.i.i = phi ptr [ %spec.select115.i.i.i, %if.end47.i.i.i ], [ %a.0.lcssa.i.i.i, %while.cond14.preheader.i.i.i ]
  %z.2103.i.i.i = phi ptr [ %z.3.i.i.i, %if.end47.i.i.i ], [ %z.0.lcssa.i.i.i, %while.cond14.preheader.i.i.i ]
  %sub21.i.i.i = sub nsw i32 0, %e2.addr.1105.i.i.i
  %cond28.i.i.i = call i32 @llvm.smin.i32(i32 %sub21.i.i.i, i32 9)
  %cmp3098.i.i.i = icmp ult ptr %a.2104.i.i.i, %z.2103.i.i.i
  br i1 %cmp3098.i.i.i, label %for.body32.lr.ph.i.i.i, label %for.end39.thread.i.i.i

for.end39.thread.i.i.i:                           ; preds = %while.body17.i.i.i
  %97 = load i32, ptr %a.2104.i.i.i, align 4, !tbaa !24
  br label %if.end47.i.i.i

for.body32.lr.ph.i.i.i:                           ; preds = %while.body17.i.i.i
  %notmask.i.i.i = shl nsw i32 -1, %cond28.i.i.i
  %sub34.i.i.i = xor i32 %notmask.i.i.i, -1
  %shr36.i.i.i = lshr i32 1000000000, %cond28.i.i.i
  br label %for.body32.i.i.i

for.body32.i.i.i:                                 ; preds = %for.body32.i.i.i, %for.body32.lr.ph.i.i.i
  %d19.0100.i.i.i = phi ptr [ %a.2104.i.i.i, %for.body32.lr.ph.i.i.i ], [ %incdec.ptr38.i.i.i, %for.body32.i.i.i ]
  %carry18.099.i.i.i = phi i32 [ 0, %for.body32.lr.ph.i.i.i ], [ %mul.i831.i.i, %for.body32.i.i.i ]
  %98 = load i32, ptr %d19.0100.i.i.i, align 4, !tbaa !24
  %and.i.i.i = and i32 %98, %sub34.i.i.i
  %shr.i830.i.i = lshr i32 %98, %cond28.i.i.i
  %add35.i.i.i = add i32 %shr.i830.i.i, %carry18.099.i.i.i
  store i32 %add35.i.i.i, ptr %d19.0100.i.i.i, align 4, !tbaa !24
  %mul.i831.i.i = mul i32 %and.i.i.i, %shr36.i.i.i
  %incdec.ptr38.i.i.i = getelementptr inbounds i32, ptr %d19.0100.i.i.i, i64 1
  %cmp30.i.i.i = icmp ult ptr %incdec.ptr38.i.i.i, %z.2103.i.i.i
  br i1 %cmp30.i.i.i, label %for.body32.i.i.i, label %for.end39.i.i.i, !llvm.loop !57

for.end39.i.i.i:                                  ; preds = %for.body32.i.i.i
  %99 = load i32, ptr %a.2104.i.i.i, align 4, !tbaa !24
  %tobool44.not.i.i.i = icmp eq i32 %mul.i831.i.i, 0
  br i1 %tobool44.not.i.i.i, label %if.end47.i.i.i, label %if.then45.i.i.i

if.then45.i.i.i:                                  ; preds = %for.end39.i.i.i
  %incdec.ptr46.i.i.i = getelementptr inbounds i32, ptr %z.2103.i.i.i, i64 1
  store i32 %mul.i831.i.i, ptr %z.2103.i.i.i, align 4, !tbaa !24
  br label %if.end47.i.i.i

if.end47.i.i.i:                                   ; preds = %if.then45.i.i.i, %for.end39.i.i.i, %for.end39.thread.i.i.i
  %spec.select.idx.pn.in.in.i.i.i = phi i32 [ %99, %if.then45.i.i.i ], [ %99, %for.end39.i.i.i ], [ %97, %for.end39.thread.i.i.i ]
  %z.3.i.i.i = phi ptr [ %incdec.ptr46.i.i.i, %if.then45.i.i.i ], [ %z.2103.i.i.i, %for.end39.i.i.i ], [ %z.2103.i.i.i, %for.end39.thread.i.i.i ]
  %spec.select.idx.pn.in.i.i.i = icmp eq i32 %spec.select.idx.pn.in.in.i.i.i, 0
  %spec.select.idx.pn.i.i.i = zext i1 %spec.select.idx.pn.in.i.i.i to i64
  %spec.select115.i.i.i = getelementptr inbounds i32, ptr %a.2104.i.i.i, i64 %spec.select.idx.pn.i.i.i
  %add48.i.i.i = add nsw i32 %cond28.i.i.i, %e2.addr.1105.i.i.i
  %cmp15.i.i.i = icmp slt i32 %add48.i.i.i, 0
  br i1 %cmp15.i.i.i, label %while.body17.i.i.i, label %double_scale.exit.i.i, !llvm.loop !58

double_scale.exit.i.i:                            ; preds = %if.end47.i.i.i, %while.cond14.preheader.i.i.i
  %z.2.lcssa.i.i.i = phi ptr [ %z.0.lcssa.i.i.i, %while.cond14.preheader.i.i.i ], [ %z.3.i.i.i, %if.end47.i.i.i ]
  %a.2.lcssa.i.i.i = phi ptr [ %a.0.lcssa.i.i.i, %while.cond14.preheader.i.i.i ], [ %spec.select115.i.i.i, %if.end47.i.i.i ]
  %cmp37.i.i = icmp ult ptr %a.2.lcssa.i.i.i, %z.2.lcssa.i.i.i
  br i1 %cmp37.i.i, label %if.then39.i.i, label %double_scale.exit.if.end45_crit_edge.i.i

double_scale.exit.if.end45_crit_edge.i.i:         ; preds = %double_scale.exit.i.i
  %.pre1165.i.i = ptrtoint ptr %z.0.i.i.i to i64
  br label %if.end45.i.i

if.then39.i.i:                                    ; preds = %double_scale.exit.i.i
  %.pre.i558.i = load i32, ptr %a.2.lcssa.i.i.i, align 4, !tbaa !24
  %sub.ptr.lhs.cast.i559.i = ptrtoint ptr %z.0.i.i.i to i64
  %sub.ptr.rhs.cast.i560.i = ptrtoint ptr %a.2.lcssa.i.i.i to i64
  %sub.ptr.sub.i561.i = sub i64 %sub.ptr.lhs.cast.i559.i, %sub.ptr.rhs.cast.i560.i
  %sub.ptr.div.i.i = lshr exact i64 %sub.ptr.sub.i561.i, 2
  %conv40.i.i = trunc i64 %sub.ptr.div.i.i to i32
  %mul.i562.i = mul nsw i32 %conv40.i.i, 9
  %cmp41.not1114.i.i = icmp ult i32 %.pre.i558.i, 10
  br i1 %cmp41.not1114.i.i, label %if.end45.i.i, label %for.body.i563.i

for.body.i563.i:                                  ; preds = %for.body.i563.i, %if.then39.i.i
  %i.01116.i.i = phi i32 [ %mul43.i.i, %for.body.i563.i ], [ 10, %if.then39.i.i ]
  %e.01115.i.i = phi i32 [ %inc.i.i, %for.body.i563.i ], [ %mul.i562.i, %if.then39.i.i ]
  %inc.i.i = add nsw i32 %e.01115.i.i, 1
  %mul43.i.i = mul nuw nsw i32 %i.01116.i.i, 10
  %cmp41.not.i.i = icmp ult i32 %.pre.i558.i, %mul43.i.i
  br i1 %cmp41.not.i.i, label %if.end45.i.i, label %for.body.i563.i, !llvm.loop !59

if.end45.i.i:                                     ; preds = %for.body.i563.i, %if.then39.i.i, %double_scale.exit.if.end45_crit_edge.i.i, %if.then39.thread.i.i
  %sub.ptr.rhs.cast55.pre-phi.i.i = phi i64 [ %.pre1165.i.i, %double_scale.exit.if.end45_crit_edge.i.i ], [ %sub.ptr.lhs.cast.i559.i, %if.then39.i.i ], [ %sub.ptr.lhs.cast1170.i.i, %if.then39.thread.i.i ], [ %sub.ptr.lhs.cast.i559.i, %for.body.i563.i ]
  %a.2.lcssa.i1099.i.i = phi ptr [ %a.2.lcssa.i.i.i, %double_scale.exit.if.end45_crit_edge.i.i ], [ %a.2.lcssa.i.i.i, %if.then39.i.i ], [ %big.i.i, %if.then39.thread.i.i ], [ %a.2.lcssa.i.i.i, %for.body.i563.i ]
  %z.2.lcssa.i1096.i.i = phi ptr [ %z.2.lcssa.i.i.i, %double_scale.exit.if.end45_crit_edge.i.i ], [ %z.2.lcssa.i.i.i, %if.then39.i.i ], [ %add.ptr.i829.i.i, %if.then39.thread.i.i ], [ %z.2.lcssa.i.i.i, %for.body.i563.i ]
  %r.0108310891095.i.i = phi ptr [ %z.0.i.i.i, %double_scale.exit.if.end45_crit_edge.i.i ], [ %z.0.i.i.i, %if.then39.i.i ], [ %big.i.i, %if.then39.thread.i.i ], [ %z.0.i.i.i, %for.body.i563.i ]
  %e.1.i.i = phi i32 [ 0, %double_scale.exit.if.end45_crit_edge.i.i ], [ %mul.i562.i, %if.then39.i.i ], [ 0, %if.then39.thread.i.i ], [ %inc.i.i, %for.body.i563.i ]
  %or.i538.i = or i32 %t.0.i, 32
  %cmp46.not.i.i = icmp eq i32 %or.i538.i, 102
  %mul48.i.i = select i1 %cmp46.not.i.i, i32 0, i32 %e.1.i.i
  %sub.i539.i = sub nsw i32 %spec.store.select.i.i, %mul48.i.i
  %cmp50.i.i = icmp eq i32 %or.i538.i, 103
  %tobool52.i.i = icmp ne i32 %spec.store.select.i.i, 0
  %100 = and i1 %tobool52.i.i, %cmp50.i.i
  %land.ext.neg.i.i = sext i1 %100 to i32
  %sub53.i.i = add i32 %sub.i539.i, %land.ext.neg.i.i
  %sub.ptr.lhs.cast54.i.i = ptrtoint ptr %z.2.lcssa.i1096.i.i to i64
  %sub.ptr.sub56.i.i = sub i64 %sub.ptr.lhs.cast54.i.i, %sub.ptr.rhs.cast55.pre-phi.i.i
  %sub.ptr.div57.i.i = lshr exact i64 %sub.ptr.sub56.i.i, 2
  %101 = trunc i64 %sub.ptr.div57.i.i to i32
  %102 = mul i32 %101, 9
  %mul60.i.i = add i32 %102, -9
  %cmp61.i.i = icmp slt i32 %sub53.i.i, %mul60.i.i
  br i1 %cmp61.i.i, label %if.then63.i554.i, label %if.end176.i.i

if.then63.i554.i:                                 ; preds = %if.end45.i.i
  %cmp64.i.i = icmp sgt i32 %sub53.i.i, -1
  br i1 %cmp64.i.i, label %cond.true66.i.i, label %cond.false67.i.i

cond.true66.i.i:                                  ; preds = %if.then63.i554.i
  %div.i557.i = udiv i32 %sub53.i.i, 9
  br label %cond.end72.i.i

cond.false67.i.i:                                 ; preds = %if.then63.i554.i
  %add69.i.i = sub i32 8, %sub53.i.i
  %div70.neg.i.i = sdiv i32 %add69.i.i, -9
  br label %cond.end72.i.i

cond.end72.i.i:                                   ; preds = %cond.false67.i.i, %cond.true66.i.i
  %cond73.i.i = phi i32 [ %div.i557.i, %cond.true66.i.i ], [ %div70.neg.i.i, %cond.false67.i.i ]
  %add.ptr76.i.i = getelementptr inbounds i32, ptr %r.0108310891095.i.i, i64 1
  %idx.ext.i.i = sext i32 %cond73.i.i to i64
  %add.ptr77.i.i = getelementptr inbounds i32, ptr %add.ptr76.i.i, i64 %idx.ext.i.i
  %mul74.neg.neg.i.i = mul nsw i32 %cond73.i.i, 9
  %reass.sub1161.i.i = sub i32 %mul74.neg.neg.i.i, %sub53.i.i
  %sub79.i.i = add i32 %reass.sub1161.i.i, 8
  %cmp801117.i.i = icmp sgt i32 %sub79.i.i, 0
  br i1 %cmp801117.i.i, label %for.body82.i.i.preheader, label %for.end86.i.i

for.body82.i.i.preheader:                         ; preds = %cond.end72.i.i
  %103 = add i32 %mul48.i.i, 7
  %104 = add i32 %103, %mul74.neg.neg.i.i
  %105 = add i32 %spec.store.select.i.i, %land.ext.neg.i.i
  %106 = sub i32 %104, %105
  %xtraiter = and i32 %reass.sub1161.i.i, 7
  %107 = icmp ult i32 %106, 7
  br i1 %107, label %for.end86.i.i.loopexit.unr-lcssa, label %for.body82.i.i.preheader.new

for.body82.i.i.preheader.new:                     ; preds = %for.body82.i.i.preheader
  %unroll_iter = and i32 %sub79.i.i, 2147483640
  br label %for.body82.i.i

for.body82.i.i:                                   ; preds = %for.body82.i.i, %for.body82.i.i.preheader.new
  %i.11118.i.i = phi i32 [ 1, %for.body82.i.i.preheader.new ], [ %mul83.i.i.7, %for.body82.i.i ]
  %niter = phi i32 [ 0, %for.body82.i.i.preheader.new ], [ %niter.next.7, %for.body82.i.i ]
  %mul83.i.i.7 = mul i32 %i.11118.i.i, 100000000
  %niter.next.7 = add i32 %niter, 8
  %niter.ncmp.7.not = icmp eq i32 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7.not, label %for.end86.i.i.loopexit.unr-lcssa, label %for.body82.i.i, !llvm.loop !60

for.end86.i.i.loopexit.unr-lcssa:                 ; preds = %for.body82.i.i, %for.body82.i.i.preheader
  %mul83.i.i.lcssa.ph = phi i32 [ undef, %for.body82.i.i.preheader ], [ %mul83.i.i.7, %for.body82.i.i ]
  %i.11118.i.i.unr = phi i32 [ 1, %for.body82.i.i.preheader ], [ %mul83.i.i.7, %for.body82.i.i ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.end86.i.i, label %for.body82.i.i.epil

for.body82.i.i.epil:                              ; preds = %for.body82.i.i.epil, %for.end86.i.i.loopexit.unr-lcssa
  %i.11118.i.i.epil = phi i32 [ %mul83.i.i.epil, %for.body82.i.i.epil ], [ %i.11118.i.i.unr, %for.end86.i.i.loopexit.unr-lcssa ]
  %epil.iter = phi i32 [ %epil.iter.next, %for.body82.i.i.epil ], [ 0, %for.end86.i.i.loopexit.unr-lcssa ]
  %mul83.i.i.epil = mul nsw i32 %i.11118.i.i.epil, 10
  %epil.iter.next = add i32 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i32 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.end86.i.i, label %for.body82.i.i.epil, !llvm.loop !61

for.end86.i.i:                                    ; preds = %for.body82.i.i.epil, %for.end86.i.i.loopexit.unr-lcssa, %cond.end72.i.i
  %i.1.lcssa.i.i = phi i32 [ 1, %cond.end72.i.i ], [ %mul83.i.i.lcssa.ph, %for.end86.i.i.loopexit.unr-lcssa ], [ %mul83.i.i.epil, %for.body82.i.i.epil ]
  %mul87.i.i = mul nsw i32 %i.1.lcssa.i.i, 10
  %108 = load i32, ptr %add.ptr77.i.i, align 4, !tbaa !24
  %rem88.i.i = urem i32 %108, %mul87.i.i
  %div130.i.i = udiv i32 %108, %mul87.i.i
  %tobool89.not.i.i = icmp eq i32 %rem88.i.i, 0
  %add.ptr91.i.i = getelementptr inbounds i32, ptr %add.ptr77.i.i, i64 1
  %cmp92.not.i.i = icmp eq ptr %add.ptr91.i.i, %z.2.lcssa.i1096.i.i
  %or.cond801.i.i = select i1 %tobool89.not.i.i, i1 %cmp92.not.i.i, i1 false
  br i1 %or.cond801.i.i, label %if.end169.i.i, label %if.then94.i.i

if.then94.i.i:                                    ; preds = %for.end86.i.i
  %div95799.i.i = lshr exact i32 %mul87.i.i, 1
  %cmp96.i.i = icmp ugt i32 %rem88.i.i, %div95799.i.i
  br i1 %cmp96.i.i, label %if.end135.thread.i.i, label %if.else99.i.i

if.else99.i.i:                                    ; preds = %if.then94.i.i
  %cmp101.i.i = icmp ult i32 %rem88.i.i, %div95799.i.i
  br i1 %cmp101.i.i, label %if.end135.thread1104.i.i, label %for.cond106.i.i

if.end135.thread1104.i.i:                         ; preds = %if.else99.i.i
  %sub1361106.i.i = sub i32 %108, %rem88.i.i
  store i32 %sub1361106.i.i, ptr %add.ptr77.i.i, align 4, !tbaa !24
  br label %if.end169.i.i

for.cond106.i.i:                                  ; preds = %for.body109.i.i, %if.else99.i.i
  %add.ptr77.pn.i.i = phi ptr [ %q.0.i.i, %for.body109.i.i ], [ %add.ptr77.i.i, %if.else99.i.i ]
  %q.0.i.i = getelementptr inbounds i32, ptr %add.ptr77.pn.i.i, i64 1
  %cmp107.not.i.i = icmp ult ptr %q.0.i.i, %z.2.lcssa.i1096.i.i
  br i1 %cmp107.not.i.i, label %for.body109.i.i, label %if.else118.i.i

for.body109.i.i:                                  ; preds = %for.cond106.i.i
  %109 = load i32, ptr %q.0.i.i, align 4, !tbaa !24
  %tobool110.not.i.i = icmp eq i32 %109, 0
  br i1 %tobool110.not.i.i, label %for.cond106.i.i, label %if.end135.thread.i.i, !llvm.loop !63

if.else118.i.i:                                   ; preds = %for.cond106.i.i
  %cmp119.i.i = icmp eq i32 %i.1.lcssa.i.i, 100000000
  br i1 %cmp119.i.i, label %if.then121.i.i, label %if.else129.i.i

if.then121.i.i:                                   ; preds = %if.else118.i.i
  %cmp122.i.i = icmp ugt ptr %add.ptr77.i.i, %a.2.lcssa.i1099.i.i
  br i1 %cmp122.i.i, label %land.rhs124.i.i, label %land.end127.i.i

land.rhs124.i.i:                                  ; preds = %if.then121.i.i
  %arrayidx.i556.i = getelementptr inbounds i32, ptr %add.ptr77.i.i, i64 -1
  %110 = load i32, ptr %arrayidx.i556.i, align 4, !tbaa !24
  %and125.i.i = and i32 %110, 1
  %tobool126.i.i = icmp ne i32 %and125.i.i, 0
  br label %land.end127.i.i

land.end127.i.i:                                  ; preds = %land.rhs124.i.i, %if.then121.i.i
  %111 = phi i1 [ false, %if.then121.i.i ], [ %tobool126.i.i, %land.rhs124.i.i ]
  %land.ext128.i.i = zext i1 %111 to i32
  br label %if.end135.i.i

if.else129.i.i:                                   ; preds = %if.else118.i.i
  %and131.i.i = and i32 %div130.i.i, 1
  br label %if.end135.i.i

if.end135.thread.i.i:                             ; preds = %for.body109.i.i, %if.then94.i.i
  %sub1361101.i.i = sub i32 %108, %rem88.i.i
  br label %if.then138.i.i

if.end135.i.i:                                    ; preds = %if.else129.i.i, %land.end127.i.i
  %up.1.i.i = phi i32 [ %land.ext128.i.i, %land.end127.i.i ], [ %and131.i.i, %if.else129.i.i ]
  %sub136.i.i = sub i32 %108, %rem88.i.i
  store i32 %sub136.i.i, ptr %add.ptr77.i.i, align 4, !tbaa !24
  %tobool137.not.i.i = icmp eq i32 %up.1.i.i, 0
  br i1 %tobool137.not.i.i, label %if.end169.i.i, label %if.then138.i.i

if.then138.i.i:                                   ; preds = %if.end135.i.i, %if.end135.thread.i.i
  %sub1361103.i.i = phi i32 [ %sub1361101.i.i, %if.end135.thread.i.i ], [ %sub136.i.i, %if.end135.i.i ]
  %add139.i.i = add i32 %sub1361103.i.i, %mul87.i.i
  store i32 %add139.i.i, ptr %add.ptr77.i.i, align 4, !tbaa !24
  %cmp1401121.i.i = icmp ugt i32 %add139.i.i, 999999999
  br i1 %cmp1401121.i.i, label %while.body.i.i, label %while.end.i555.i

while.body.i.i:                                   ; preds = %if.end147.i.i, %if.then138.i.i
  %d.01123.i.i = phi ptr [ %incdec.ptr142.i.i, %if.end147.i.i ], [ %add.ptr77.i.i, %if.then138.i.i ]
  %a.11122.i.i = phi ptr [ %a.2.i.i, %if.end147.i.i ], [ %a.2.lcssa.i1099.i.i, %if.then138.i.i ]
  %incdec.ptr142.i.i = getelementptr inbounds i32, ptr %d.01123.i.i, i64 -1
  store i32 0, ptr %d.01123.i.i, align 4, !tbaa !24
  %cmp143.i.i = icmp ult ptr %incdec.ptr142.i.i, %a.11122.i.i
  br i1 %cmp143.i.i, label %if.then145.i.i, label %if.end147.i.i

if.then145.i.i:                                   ; preds = %while.body.i.i
  %incdec.ptr146.i.i = getelementptr inbounds i32, ptr %a.11122.i.i, i64 -1
  store i32 0, ptr %incdec.ptr146.i.i, align 4, !tbaa !24
  br label %if.end147.i.i

if.end147.i.i:                                    ; preds = %if.then145.i.i, %while.body.i.i
  %a.2.i.i = phi ptr [ %incdec.ptr146.i.i, %if.then145.i.i ], [ %a.11122.i.i, %while.body.i.i ]
  %112 = load i32, ptr %incdec.ptr142.i.i, align 4, !tbaa !24
  %inc148.i.i = add i32 %112, 1
  store i32 %inc148.i.i, ptr %incdec.ptr142.i.i, align 4, !tbaa !24
  %cmp140.i.i = icmp ugt i32 %inc148.i.i, 999999999
  br i1 %cmp140.i.i, label %while.body.i.i, label %while.end.i555.i, !llvm.loop !64

while.end.i555.i:                                 ; preds = %if.end147.i.i, %if.then138.i.i
  %a.1.lcssa.i.i = phi ptr [ %a.2.lcssa.i1099.i.i, %if.then138.i.i ], [ %a.2.i.i, %if.end147.i.i ]
  %d.0.lcssa.i.i = phi ptr [ %add.ptr77.i.i, %if.then138.i.i ], [ %incdec.ptr142.i.i, %if.end147.i.i ]
  %cmp149.i.i = icmp ult ptr %a.1.lcssa.i.i, %z.2.lcssa.i1096.i.i
  br i1 %cmp149.i.i, label %if.then151.i.i, label %if.end169.i.i

if.then151.i.i:                                   ; preds = %while.end.i555.i
  %sub.ptr.rhs.cast153.i.i = ptrtoint ptr %a.1.lcssa.i.i to i64
  %sub.ptr.sub154.i.i = sub i64 %sub.ptr.rhs.cast55.pre-phi.i.i, %sub.ptr.rhs.cast153.i.i
  %sub.ptr.div155.i.i = lshr exact i64 %sub.ptr.sub154.i.i, 2
  %conv156.i.i = trunc i64 %sub.ptr.div155.i.i to i32
  %mul157.i.i = mul nsw i32 %conv156.i.i, 9
  %113 = load i32, ptr %a.1.lcssa.i.i, align 4, !tbaa !24
  %cmp159.not1126.i.i = icmp ult i32 %113, 10
  br i1 %cmp159.not1126.i.i, label %if.end169.i.i, label %for.body161.i.i

for.body161.i.i:                                  ; preds = %for.body161.i.i, %if.then151.i.i
  %i.21128.i.i = phi i32 [ %mul164.i.i, %for.body161.i.i ], [ 10, %if.then151.i.i ]
  %e.21127.i.i = phi i32 [ %inc162.i.i, %for.body161.i.i ], [ %mul157.i.i, %if.then151.i.i ]
  %inc162.i.i = add nsw i32 %e.21127.i.i, 1
  %mul164.i.i = mul nuw nsw i32 %i.21128.i.i, 10
  %cmp159.not.i.i = icmp ult i32 %113, %mul164.i.i
  br i1 %cmp159.not.i.i, label %if.end169.i.i, label %for.body161.i.i, !llvm.loop !65

if.end169.i.i:                                    ; preds = %for.body161.i.i, %if.then151.i.i, %while.end.i555.i, %if.end135.i.i, %if.end135.thread1104.i.i, %for.end86.i.i
  %a.3.i.i = phi ptr [ %a.2.lcssa.i1099.i.i, %for.end86.i.i ], [ %a.2.lcssa.i1099.i.i, %if.end135.i.i ], [ %a.1.lcssa.i.i, %while.end.i555.i ], [ %a.2.lcssa.i1099.i.i, %if.end135.thread1104.i.i ], [ %a.1.lcssa.i.i, %if.then151.i.i ], [ %a.1.lcssa.i.i, %for.body161.i.i ]
  %d.2.i.i = phi ptr [ %add.ptr77.i.i, %for.end86.i.i ], [ %add.ptr77.i.i, %if.end135.i.i ], [ %d.0.lcssa.i.i, %while.end.i555.i ], [ %add.ptr77.i.i, %if.end135.thread1104.i.i ], [ %d.0.lcssa.i.i, %if.then151.i.i ], [ %d.0.lcssa.i.i, %for.body161.i.i ]
  %e.4.i.i = phi i32 [ %e.1.i.i, %for.end86.i.i ], [ %e.1.i.i, %if.end135.i.i ], [ 0, %while.end.i555.i ], [ %e.1.i.i, %if.end135.thread1104.i.i ], [ %mul157.i.i, %if.then151.i.i ], [ %inc162.i.i, %for.body161.i.i ]
  %add.ptr170.i.i = getelementptr inbounds i32, ptr %d.2.i.i, i64 1
  %cmp171.i.i = icmp ugt ptr %z.2.lcssa.i1096.i.i, %add.ptr170.i.i
  %spec.select1108.i.i = select i1 %cmp171.i.i, ptr %add.ptr170.i.i, ptr %z.2.lcssa.i1096.i.i
  br label %if.end176.i.i

if.end176.i.i:                                    ; preds = %if.end169.i.i, %if.end45.i.i
  %z.0.i.i = phi ptr [ %z.2.lcssa.i1096.i.i, %if.end45.i.i ], [ %spec.select1108.i.i, %if.end169.i.i ]
  %a.4.i.i = phi ptr [ %a.2.lcssa.i1099.i.i, %if.end45.i.i ], [ %a.3.i.i, %if.end169.i.i ]
  %e.5.i.i = phi i32 [ %e.1.i.i, %if.end45.i.i ], [ %e.4.i.i, %if.end169.i.i ]
  br label %for.cond177.i.i

for.cond177.i.i:                                  ; preds = %land.rhs180.i.i, %if.end176.i.i
  %z.1.i.i = phi ptr [ %z.0.i.i, %if.end176.i.i ], [ %arrayidx181.i.i, %land.rhs180.i.i ]
  %cmp178.i.i = icmp ugt ptr %z.1.i.i, %a.4.i.i
  br i1 %cmp178.i.i, label %land.rhs180.i.i, label %for.end188.i.i

land.rhs180.i.i:                                  ; preds = %for.cond177.i.i
  %arrayidx181.i.i = getelementptr inbounds i32, ptr %z.1.i.i, i64 -1
  %114 = load i32, ptr %arrayidx181.i.i, align 4, !tbaa !24
  %tobool182.not.i.i = icmp eq i32 %114, 0
  br i1 %tobool182.not.i.i, label %for.cond177.i.i, label %for.end188.i.i, !llvm.loop !66

for.end188.i.i:                                   ; preds = %land.rhs180.i.i, %for.cond177.i.i
  br i1 %cmp50.i.i, label %if.then192.i.i, label %for.end188.if.end341_crit_edge.i.i

for.end188.if.end341_crit_edge.i.i:               ; preds = %for.end188.i.i
  %.pre1166.i.i = and i32 %fl.2.i, 8
  br label %if.end341.i.i

if.then192.i.i:                                   ; preds = %for.end188.i.i
  %spec.select.i553.i = select i1 %tobool52.i.i, i32 %spec.store.select.i.i, i32 1
  %cmp197.i.i = icmp sgt i32 %spec.select.i553.i, %e.5.i.i
  %cmp200.i.i = icmp sgt i32 %e.5.i.i, -5
  %or.cond591.i.i = and i1 %cmp197.i.i, %cmp200.i.i
  %add203.neg.i.i = xor i32 %e.5.i.i, -1
  %add203.neg.pn.i.i = select i1 %or.cond591.i.i, i32 %add203.neg.i.i, i32 -1
  %t.addr.0.v.i.i = select i1 %or.cond591.i.i, i32 -1, i32 -2
  %t.addr.0.i.i = add nsw i32 %t.addr.0.v.i.i, %t.0.i
  %p.addr.1.i.i = add i32 %add203.neg.pn.i.i, %spec.select.i553.i
  %and209.i.i = and i32 %fl.2.i, 8
  %tobool210.not.i.i = icmp eq i32 %and209.i.i, 0
  br i1 %tobool210.not.i.i, label %if.then211.i.i, label %if.end341.i.i

if.then211.i.i:                                   ; preds = %if.then192.i.i
  br i1 %cmp178.i.i, label %land.lhs.true214.i.i, label %if.end229.i.i

land.lhs.true214.i.i:                             ; preds = %if.then211.i.i
  %arrayidx215.i.i = getelementptr inbounds i32, ptr %z.1.i.i, i64 -1
  %115 = load i32, ptr %arrayidx215.i.i, align 4, !tbaa !24
  %tobool216.not.i.i = icmp eq i32 %115, 0
  br i1 %tobool216.not.i.i, label %if.end229.i.i, label %for.cond218.preheader.i.i

for.cond218.preheader.i.i:                        ; preds = %land.lhs.true214.i.i
  %rem2201130.i.i = urem i32 %115, 10
  %cmp2211131.i.i = icmp eq i32 %rem2201130.i.i, 0
  br i1 %cmp2211131.i.i, label %for.inc224.i.i, label %if.end229.i.i

for.inc224.i.i:                                   ; preds = %for.inc224.i.i, %for.cond218.preheader.i.i
  %j.01133.i.i = phi i32 [ %inc226.i.i, %for.inc224.i.i ], [ 0, %for.cond218.preheader.i.i ]
  %i.31132.i.i = phi i32 [ %mul225.i.i, %for.inc224.i.i ], [ 10, %for.cond218.preheader.i.i ]
  %mul225.i.i = mul nuw nsw i32 %i.31132.i.i, 10
  %inc226.i.i = add nuw nsw i32 %j.01133.i.i, 1
  %rem220.i.i = urem i32 %115, %mul225.i.i
  %cmp221.i.i = icmp eq i32 %rem220.i.i, 0
  br i1 %cmp221.i.i, label %for.inc224.i.i, label %if.end229.loopexit.i.i, !llvm.loop !67

if.end229.loopexit.i.i:                           ; preds = %for.inc224.i.i
  %inc226.neg.i.i = xor i32 %j.01133.i.i, -1
  br label %if.end229.i.i

if.end229.i.i:                                    ; preds = %if.end229.loopexit.i.i, %for.cond218.preheader.i.i, %land.lhs.true214.i.i, %if.then211.i.i
  %j.1.neg1162.i.i = phi i32 [ -9, %land.lhs.true214.i.i ], [ -9, %if.then211.i.i ], [ 0, %for.cond218.preheader.i.i ], [ %inc226.neg.i.i, %if.end229.loopexit.i.i ]
  %116 = and i32 %t.addr.0.i.i, 71
  %cmp231.i.i = icmp eq i32 %116, 70
  %sub.ptr.lhs.cast234.i.i = ptrtoint ptr %z.1.i.i to i64
  %sub.ptr.sub236.i.i = sub i64 %sub.ptr.lhs.cast234.i.i, %sub.ptr.rhs.cast55.pre-phi.i.i
  %sub.ptr.div237.i.i = lshr exact i64 %sub.ptr.sub236.i.i, 2
  %117 = trunc i64 %sub.ptr.div237.i.i to i32
  %118 = mul i32 %117, 9
  br i1 %cmp231.i.i, label %if.then233.i.i, label %if.else284.i.i

if.then233.i.i:                                   ; preds = %if.end229.i.i
  %mul240.i.i = add i32 %118, -9
  %sub241.i.i = add i32 %mul240.i.i, %j.1.neg1162.i.i
  %spec.select802.i.i = call i32 @llvm.smax.i32(i32 %sub241.i.i, i32 0)
  %spec.select1109.i.i = call i32 @llvm.smin.i32(i32 %p.addr.1.i.i, i32 %spec.select802.i.i)
  br label %if.end341.i.i

if.else284.i.i:                                   ; preds = %if.end229.i.i
  %mul291.i.i = add i32 %e.5.i.i, -9
  %add292.i.i = add i32 %mul291.i.i, %118
  %sub293.i.i = add i32 %add292.i.i, %j.1.neg1162.i.i
  %spec.select804.i.i = call i32 @llvm.smax.i32(i32 %sub293.i.i, i32 0)
  %spec.select1110.i.i = call i32 @llvm.smin.i32(i32 %p.addr.1.i.i, i32 %spec.select804.i.i)
  br label %if.end341.i.i

if.end341.i.i:                                    ; preds = %if.else284.i.i, %if.then233.i.i, %if.then192.i.i, %for.end188.if.end341_crit_edge.i.i
  %and343.pre-phi.i.i = phi i32 [ %.pre1166.i.i, %for.end188.if.end341_crit_edge.i.i ], [ 0, %if.else284.i.i ], [ 0, %if.then233.i.i ], [ %and209.i.i, %if.then192.i.i ]
  %p.addr.2.i.i = phi i32 [ %spec.store.select.i.i, %for.end188.if.end341_crit_edge.i.i ], [ %spec.select1110.i.i, %if.else284.i.i ], [ %spec.select1109.i.i, %if.then233.i.i ], [ %p.addr.1.i.i, %if.then192.i.i ]
  %t.addr.1.i.i = phi i32 [ %t.0.i, %for.end188.if.end341_crit_edge.i.i ], [ %t.addr.0.i.i, %if.else284.i.i ], [ %t.addr.0.i.i, %if.then233.i.i ], [ %t.addr.0.i.i, %if.then192.i.i ]
  %tobool344.not.i.i = icmp ne i32 %and343.pre-phi.i.i, 0
  %119 = or i32 %p.addr.2.i.i, %and343.pre-phi.i.i
  %120 = icmp ne i32 %119, 0
  %sub345.i.i = select i1 %120, i32 2147483645, i32 2147483646
  %cmp346.i.i = icmp sgt i32 %p.addr.2.i.i, %sub345.i.i
  br i1 %cmp346.i.i, label %fmt_fp.exit.i, label %if.end349.i.i

if.end349.i.i:                                    ; preds = %if.end341.i.i
  %lor.ext.i.i = zext i1 %120 to i32
  %121 = and i32 %t.addr.1.i.i, -33
  %cmp359.i.i = icmp eq i32 %121, 70
  br i1 %cmp359.i.i, label %if.then361.i.i, label %if.else367.i.i

if.then361.i.i:                                   ; preds = %if.end349.i.i
  %add365.i.i = call i32 @llvm.smax.i32(i32 %e.5.i.i, i32 0)
  br label %if.end399.i.i

if.else367.i.i:                                   ; preds = %if.end349.i.i
  %cmp368.i.i = icmp slt i32 %e.5.i.i, 0
  %cond376.v.i.i = call i32 @llvm.abs.i32(i32 %e.5.i.i, i1 true)
  %cmp330.i.i540.i = icmp ugt i32 %cond376.v.i.i, 9
  br i1 %cmp330.i.i540.i, label %for.body5.i.i548.i, label %for.end12.i.i541.i

for.body5.i.i548.i:                               ; preds = %for.body5.i.i548.i, %if.else367.i.i
  %y.032.i.i549.i = phi i32 [ %div11.i.i551.i, %for.body5.i.i548.i ], [ %cond376.v.i.i, %if.else367.i.i ]
  %s.addr.131.i.i.i = phi ptr [ %incdec.ptr9.i.i.i, %for.body5.i.i548.i ], [ %add.ptr.i526.i, %if.else367.i.i ]
  %rem6.i.i550.i = urem i32 %y.032.i.i549.i, 10
  %122 = trunc i32 %rem6.i.i550.i to i8
  %conv8.i845.i.i = or disjoint i8 %122, 48
  %incdec.ptr9.i.i.i = getelementptr inbounds i8, ptr %s.addr.131.i.i.i, i64 -1
  store i8 %conv8.i845.i.i, ptr %incdec.ptr9.i.i.i, align 1, !tbaa !4
  %div11.i.i551.i = udiv i32 %y.032.i.i549.i, 10
  %cmp3.i.i552.i = icmp ugt i32 %y.032.i.i549.i, 99
  br i1 %cmp3.i.i552.i, label %for.body5.i.i548.i, label %for.end12.i.i541.i, !llvm.loop !51

for.end12.i.i541.i:                               ; preds = %for.body5.i.i548.i, %if.else367.i.i
  %s.addr.1.lcssa.i.i.i = phi ptr [ %add.ptr.i526.i, %if.else367.i.i ], [ %incdec.ptr9.i.i.i, %for.body5.i.i548.i ]
  %y.0.lcssa.i.i542.i = phi i32 [ %cond376.v.i.i, %if.else367.i.i ], [ %div11.i.i551.i, %for.body5.i.i548.i ]
  %tobool.not.i842.i.i = icmp eq i32 %y.0.lcssa.i.i542.i, 0
  br i1 %tobool.not.i842.i.i, label %fmt_u.exit.i544.i, label %if.then.i843.i.i

if.then.i843.i.i:                                 ; preds = %for.end12.i.i541.i
  %123 = trunc i32 %y.0.lcssa.i.i542.i to i8
  %conv14.i.i543.i = or disjoint i8 %123, 48
  %incdec.ptr15.i.i.i = getelementptr inbounds i8, ptr %s.addr.1.lcssa.i.i.i, i64 -1
  store i8 %conv14.i.i543.i, ptr %incdec.ptr15.i.i.i, align 1, !tbaa !4
  br label %fmt_u.exit.i544.i

fmt_u.exit.i544.i:                                ; preds = %if.then.i843.i.i, %for.end12.i.i541.i
  %s.addr.2.i.i.i = phi ptr [ %incdec.ptr15.i.i.i, %if.then.i843.i.i ], [ %s.addr.1.lcssa.i.i.i, %for.end12.i.i541.i ]
  %sub.ptr.rhs.cast3801135.i.i = ptrtoint ptr %s.addr.2.i.i.i to i64
  %sub.ptr.sub3811136.i.i = sub i64 %sub.ptr.lhs.cast379.i.i, %sub.ptr.rhs.cast3801135.i.i
  %cmp3821137.i.i = icmp slt i64 %sub.ptr.sub3811136.i.i, 2
  br i1 %cmp3821137.i.i, label %while.body384.i.i.preheader, label %while.end386.i.i

while.body384.i.i.preheader:                      ; preds = %fmt_u.exit.i544.i
  %124 = add i64 %sub.ptr.rhs.cast3801135.i.i, 2
  %reass.sub88 = sub i64 %sub.ptr.rhs.cast3801135.i.i, %ebuf.i525.i73
  %125 = add i64 %reass.sub88, -15
  %xtraiter74 = and i64 %124, 7
  %lcmp.mod75.not = icmp eq i64 %xtraiter74, 0
  br i1 %lcmp.mod75.not, label %while.body384.i.i.prol.loopexit, label %while.body384.i.i.prol

while.body384.i.i.prol:                           ; preds = %while.body384.i.i.prol, %while.body384.i.i.preheader
  %estr.01138.i.i.prol = phi ptr [ %incdec.ptr385.i.i.prol, %while.body384.i.i.prol ], [ %s.addr.2.i.i.i, %while.body384.i.i.preheader ]
  %prol.iter = phi i64 [ %prol.iter.next, %while.body384.i.i.prol ], [ 0, %while.body384.i.i.preheader ]
  %incdec.ptr385.i.i.prol = getelementptr inbounds i8, ptr %estr.01138.i.i.prol, i64 -1
  store i8 48, ptr %incdec.ptr385.i.i.prol, align 1, !tbaa !4
  %prol.iter.next = add i64 %prol.iter, 1
  %prol.iter.cmp.not = icmp eq i64 %prol.iter.next, %xtraiter74
  br i1 %prol.iter.cmp.not, label %while.body384.i.i.prol.loopexit, label %while.body384.i.i.prol, !llvm.loop !68

while.body384.i.i.prol.loopexit:                  ; preds = %while.body384.i.i.prol, %while.body384.i.i.preheader
  %estr.01138.i.i.unr = phi ptr [ %s.addr.2.i.i.i, %while.body384.i.i.preheader ], [ %incdec.ptr385.i.i.prol, %while.body384.i.i.prol ]
  %incdec.ptr385.i.i.lcssa.unr = phi ptr [ undef, %while.body384.i.i.preheader ], [ %incdec.ptr385.i.i.prol, %while.body384.i.i.prol ]
  %126 = icmp ult i64 %125, 7
  br i1 %126, label %while.end386.i.i, label %while.body384.i.i

while.body384.i.i:                                ; preds = %while.body384.i.i, %while.body384.i.i.prol.loopexit
  %estr.01138.i.i = phi ptr [ %incdec.ptr385.i.i.7, %while.body384.i.i ], [ %estr.01138.i.i.unr, %while.body384.i.i.prol.loopexit ]
  %incdec.ptr385.i.i = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -1
  store i8 48, ptr %incdec.ptr385.i.i, align 1, !tbaa !4
  %incdec.ptr385.i.i.1 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -2
  store i8 48, ptr %incdec.ptr385.i.i.1, align 1, !tbaa !4
  %incdec.ptr385.i.i.2 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -3
  store i8 48, ptr %incdec.ptr385.i.i.2, align 1, !tbaa !4
  %incdec.ptr385.i.i.3 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -4
  store i8 48, ptr %incdec.ptr385.i.i.3, align 1, !tbaa !4
  %incdec.ptr385.i.i.4 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -5
  store i8 48, ptr %incdec.ptr385.i.i.4, align 1, !tbaa !4
  %incdec.ptr385.i.i.5 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -6
  store i8 48, ptr %incdec.ptr385.i.i.5, align 1, !tbaa !4
  %incdec.ptr385.i.i.6 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -7
  store i8 48, ptr %incdec.ptr385.i.i.6, align 1, !tbaa !4
  %incdec.ptr385.i.i.7 = getelementptr inbounds i8, ptr %estr.01138.i.i, i64 -8
  store i8 48, ptr %incdec.ptr385.i.i.7, align 1, !tbaa !4
  %sub.ptr.rhs.cast380.i.i.7 = ptrtoint ptr %incdec.ptr385.i.i.7 to i64
  %sub.ptr.sub381.i.i.7 = sub i64 %sub.ptr.lhs.cast379.i.i, %sub.ptr.rhs.cast380.i.i.7
  %cmp382.i.i.7 = icmp slt i64 %sub.ptr.sub381.i.i.7, 2
  br i1 %cmp382.i.i.7, label %while.body384.i.i, label %while.end386.i.i, !llvm.loop !69

while.end386.i.i:                                 ; preds = %while.body384.i.i, %while.body384.i.i.prol.loopexit, %fmt_u.exit.i544.i
  %estr.0.lcssa.i.i = phi ptr [ %s.addr.2.i.i.i, %fmt_u.exit.i544.i ], [ %incdec.ptr385.i.i.lcssa.unr, %while.body384.i.i.prol.loopexit ], [ %incdec.ptr385.i.i.7, %while.body384.i.i ]
  %conv390.i.i = select i1 %cmp368.i.i, i8 45, i8 43
  %incdec.ptr391.i.i = getelementptr inbounds i8, ptr %estr.0.lcssa.i.i, i64 -1
  store i8 %conv390.i.i, ptr %incdec.ptr391.i.i, align 1, !tbaa !4
  %conv392.i.i = trunc i32 %t.addr.1.i.i to i8
  %incdec.ptr393.i.i = getelementptr inbounds i8, ptr %estr.0.lcssa.i.i, i64 -2
  store i8 %conv392.i.i, ptr %incdec.ptr393.i.i, align 1, !tbaa !4
  %sub.ptr.rhs.cast395.i.i = ptrtoint ptr %incdec.ptr393.i.i to i64
  %sub.ptr.sub396.i.i = sub i64 %sub.ptr.lhs.cast379.i.i, %sub.ptr.rhs.cast395.i.i
  %conv397.i.i = trunc i64 %sub.ptr.sub396.i.i to i32
  br label %if.end399.i.i

if.end399.i.i:                                    ; preds = %while.end386.i.i, %if.then361.i.i
  %conv397.pn.i.i = phi i32 [ %conv397.i.i, %while.end386.i.i ], [ %add365.i.i, %if.then361.i.i ]
  %estr.1.i.i = phi ptr [ %incdec.ptr393.i.i, %while.end386.i.i ], [ undef, %if.then361.i.i ]
  %add350.i.i = add nuw nsw i32 %pl.0.i533.i, 1
  %add357.i.i = add i32 %add350.i.i, %p.addr.2.i.i
  %l.0.i.i = add i32 %add357.i.i, %lor.ext.i.i
  %add400.i.i = add i32 %l.0.i.i, %conv397.pn.i.i
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add400.i.i, i32 noundef %fl.2.i) #14
  %conv401.i.i = zext nneg i32 %pl.0.i533.i to i64
  %tobool.not.i846.i.i = icmp eq i32 %pl.0.i533.i, 0
  br i1 %tobool.not.i846.i.i, label %emit.exit862.i.i, label %if.end.i847.i.i

if.end.i847.i.i:                                  ; preds = %if.end399.i.i
  %127 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i848.i.i = icmp eq ptr %127, null
  br i1 %tobool1.not.i848.i.i, label %if.else.i861.i.i, label %if.then2.i849.i.i

if.then2.i849.i.i:                                ; preds = %if.end.i847.i.i
  %128 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %129 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i852.i.i = icmp ult i64 %128, %129
  br i1 %cmp.i852.i.i, label %if.then3.i856.i.i, label %if.end11.i853.i.i

if.then3.i856.i.i:                                ; preds = %if.then2.i849.i.i
  %130 = xor i64 %128, -1
  %sub6.i857.i.i = add i64 %129, %130
  %cond.i858.i.i = call i64 @llvm.umin.i64(i64 %sub6.i857.i.i, i64 %conv401.i.i)
  %add.ptr.i859.i.i = getelementptr inbounds i8, ptr %127, i64 %128
  %call.i860.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i859.i.i, ptr noundef %prefix.0.i532.i, i64 noundef %cond.i858.i.i) #12
  br label %if.end11.i853.i.i

if.else.i861.i.i:                                 ; preds = %if.end.i847.i.i
  call void @__msil_write(ptr noundef %prefix.0.i532.i, i64 noundef %conv401.i.i) #12
  br label %if.end11.i853.i.i

if.end11.i853.i.i:                                ; preds = %if.else.i861.i.i, %if.then3.i856.i.i, %if.then2.i849.i.i
  %131 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i855.i.i = add i64 %131, %conv401.i.i
  store i64 %add.i855.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %emit.exit862.i.i

emit.exit862.i.i:                                 ; preds = %if.end11.i853.i.i, %if.end399.i.i
  %xor402.i.i = xor i32 %fl.2.i, 65536
  call fastcc void @pad(ptr noundef %o, i8 noundef 48, i32 noundef %w.0.i, i32 noundef %add400.i.i, i32 noundef %xor402.i.i) #14
  br i1 %cmp359.i.i, label %if.then407.i.i, label %if.else485.i.i

if.then407.i.i:                                   ; preds = %emit.exit862.i.i
  %cmp408.i.i = icmp ugt ptr %a.4.i.i, %r.0108310891095.i.i
  %spec.select1111.i.i = select i1 %cmp408.i.i, ptr %r.0108310891095.i.i, ptr %a.4.i.i
  br label %for.body415.i.i

for.body415.i.i:                                  ; preds = %if.end11.i890.i.i, %if.then407.i.i
  %d.31152.i.i = phi ptr [ %spec.select1111.i.i, %if.then407.i.i ], [ %incdec.ptr445.i.i, %if.end11.i890.i.i ]
  %132 = load i32, ptr %d.31152.i.i, align 4, !tbaa !24
  %cmp330.i864.i.i = icmp ugt i32 %132, 9
  br i1 %cmp330.i864.i.i, label %for.body5.i874.i.i, label %for.end12.i865.i.i

for.body5.i874.i.i:                               ; preds = %for.body5.i874.i.i, %for.body415.i.i
  %y.032.i875.i.i = phi i32 [ %div11.i880.i.i, %for.body5.i874.i.i ], [ %132, %for.body415.i.i ]
  %s.addr.131.i876.idx.i.i = phi i64 [ %s.addr.131.i876.add.i.i, %for.body5.i874.i.i ], [ 9, %for.body415.i.i ]
  %rem6.i877.i.i = urem i32 %y.032.i875.i.i, 10
  %133 = trunc i32 %rem6.i877.i.i to i8
  %conv8.i878.i.i = or disjoint i8 %133, 48
  %s.addr.131.i876.add.i.i = add nsw i64 %s.addr.131.i876.idx.i.i, -1
  %incdec.ptr9.i879.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.addr.131.i876.add.i.i
  store i8 %conv8.i878.i.i, ptr %incdec.ptr9.i879.ptr.i.i, align 1, !tbaa !4
  %div11.i880.i.i = udiv i32 %y.032.i875.i.i, 10
  %cmp3.i881.i.i = icmp ugt i32 %y.032.i875.i.i, 99
  br i1 %cmp3.i881.i.i, label %for.body5.i874.i.i, label %for.end12.i865.i.i, !llvm.loop !51

for.end12.i865.i.i:                               ; preds = %for.body5.i874.i.i, %for.body415.i.i
  %s.addr.1.lcssa.i866.idx.i.i = phi i64 [ 9, %for.body415.i.i ], [ %s.addr.131.i876.add.i.i, %for.body5.i874.i.i ]
  %y.0.lcssa.i867.i.i = phi i32 [ %132, %for.body415.i.i ], [ %div11.i880.i.i, %for.body5.i874.i.i ]
  %tobool.not.i868.i.i = icmp eq i32 %y.0.lcssa.i867.i.i, 0
  br i1 %tobool.not.i868.i.i, label %fmt_u.exit882.i.i, label %if.then.i869.i.i

if.then.i869.i.i:                                 ; preds = %for.end12.i865.i.i
  %134 = trunc i32 %y.0.lcssa.i867.i.i to i8
  %conv14.i870.i.i = or disjoint i8 %134, 48
  %s.addr.1.lcssa.i866.add.i.i = add nsw i64 %s.addr.1.lcssa.i866.idx.i.i, -1
  %incdec.ptr15.i871.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.addr.1.lcssa.i866.add.i.i
  store i8 %conv14.i870.i.i, ptr %incdec.ptr15.i871.ptr.i.i, align 1, !tbaa !4
  br label %fmt_u.exit882.i.i

fmt_u.exit882.i.i:                                ; preds = %if.then.i869.i.i, %for.end12.i865.i.i
  %s.addr.2.i873.idx.i.i = phi i64 [ %s.addr.1.lcssa.i866.add.i.i, %if.then.i869.i.i ], [ %s.addr.1.lcssa.i866.idx.i.i, %for.end12.i865.i.i ]
  %cmp420.not.i.i = icmp eq ptr %d.31152.i.i, %spec.select1111.i.i
  br i1 %cmp420.not.i.i, label %if.else430.i.i, label %while.cond423.preheader.i.i

while.cond423.preheader.i.i:                      ; preds = %fmt_u.exit882.i.i
  %cmp4251149.i.i = icmp sgt i64 %s.addr.2.i873.idx.i.i, 0
  br i1 %cmp4251149.i.i, label %while.body427.i.i.preheader, label %if.end.i884.i.i

while.body427.i.i.preheader:                      ; preds = %while.cond423.preheader.i.i
  %xtraiter81 = and i64 %s.addr.2.i873.idx.i.i, 7
  %lcmp.mod82.not = icmp eq i64 %xtraiter81, 0
  br i1 %lcmp.mod82.not, label %while.body427.i.i.prol.loopexit, label %while.body427.i.i.prol

while.body427.i.i.prol:                           ; preds = %while.body427.i.i.prol, %while.body427.i.i.preheader
  %s.0.idx1150.i.i.prol = phi i64 [ %s.0.add.i.i.prol, %while.body427.i.i.prol ], [ %s.addr.2.i873.idx.i.i, %while.body427.i.i.preheader ]
  %prol.iter83 = phi i64 [ %prol.iter83.next, %while.body427.i.i.prol ], [ 0, %while.body427.i.i.preheader ]
  %s.0.add.i.i.prol = add nsw i64 %s.0.idx1150.i.i.prol, -1
  %incdec.ptr428.ptr.i.i.prol = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.0.add.i.i.prol
  store i8 48, ptr %incdec.ptr428.ptr.i.i.prol, align 1, !tbaa !4
  %prol.iter83.next = add i64 %prol.iter83, 1
  %prol.iter83.cmp.not = icmp eq i64 %prol.iter83.next, %xtraiter81
  br i1 %prol.iter83.cmp.not, label %while.body427.i.i.prol.loopexit, label %while.body427.i.i.prol, !llvm.loop !70

while.body427.i.i.prol.loopexit:                  ; preds = %while.body427.i.i.prol, %while.body427.i.i.preheader
  %s.0.idx1150.i.i.unr = phi i64 [ %s.addr.2.i873.idx.i.i, %while.body427.i.i.preheader ], [ %s.0.add.i.i.prol, %while.body427.i.i.prol ]
  %135 = icmp ult i64 %s.addr.2.i873.idx.i.i, 8
  br i1 %135, label %if.end.i884.i.i, label %while.body427.i.i

while.body427.i.i:                                ; preds = %while.body427.i.i, %while.body427.i.i.prol.loopexit
  %s.0.idx1150.i.i = phi i64 [ %s.0.add.i.i.7, %while.body427.i.i ], [ %s.0.idx1150.i.i.unr, %while.body427.i.i.prol.loopexit ]
  %gep = getelementptr i8, ptr %invariant.gep, i64 %s.0.idx1150.i.i
  store i8 48, ptr %gep, align 1, !tbaa !4
  %gep96 = getelementptr i8, ptr %invariant.gep95, i64 %s.0.idx1150.i.i
  store i8 48, ptr %gep96, align 1, !tbaa !4
  %gep98 = getelementptr i8, ptr %invariant.gep97, i64 %s.0.idx1150.i.i
  store i8 48, ptr %gep98, align 1, !tbaa !4
  %gep100 = getelementptr i8, ptr %invariant.gep99, i64 %s.0.idx1150.i.i
  store i8 48, ptr %gep100, align 1, !tbaa !4
  %gep102 = getelementptr i8, ptr %invariant.gep101, i64 %s.0.idx1150.i.i
  store i8 48, ptr %gep102, align 1, !tbaa !4
  %gep104 = getelementptr i8, ptr %invariant.gep103, i64 %s.0.idx1150.i.i
  store i8 48, ptr %gep104, align 1, !tbaa !4
  %s.0.add.i.i.6 = add nsw i64 %s.0.idx1150.i.i, -7
  %incdec.ptr428.ptr.i.i.6 = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.0.add.i.i.6
  store i8 48, ptr %incdec.ptr428.ptr.i.i.6, align 1, !tbaa !4
  %s.0.add.i.i.7 = add nsw i64 %s.0.idx1150.i.i, -8
  %incdec.ptr428.ptr.i.i.7 = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.0.add.i.i.7
  store i8 48, ptr %incdec.ptr428.ptr.i.i.7, align 1, !tbaa !4
  %cmp425.i.i.7 = icmp ugt i64 %s.0.add.i.i.6, 1
  br i1 %cmp425.i.i.7, label %while.body427.i.i, label %if.end.i884.i.i, !llvm.loop !71

if.else430.i.i:                                   ; preds = %fmt_u.exit882.i.i
  %cmp433.i.i = icmp eq i64 %s.addr.2.i873.idx.i.i, 9
  br i1 %cmp433.i.i, label %if.then435.i.i, label %if.end.i884.i.i

if.then435.i.i:                                   ; preds = %if.else430.i.i
  store i8 48, ptr %incdec.ptr436.ptr.i.i, align 8, !tbaa !4
  br label %if.end.i884.i.i

if.end.i884.i.i:                                  ; preds = %if.then435.i.i, %if.else430.i.i, %while.body427.i.i, %while.body427.i.i.prol.loopexit, %while.cond423.preheader.i.i
  %s.addr.2.i873.idx.sink1183.i.i = phi i64 [ %s.addr.2.i873.idx.i.i, %while.cond423.preheader.i.i ], [ 8, %if.then435.i.i ], [ %s.addr.2.i873.idx.i.i, %if.else430.i.i ], [ 0, %while.body427.i.i ], [ 0, %while.body427.i.i.prol.loopexit ]
  %s.1.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.addr.2.i873.idx.sink1183.i.i
  %gepdiff.i547.i = sub nsw i64 9, %s.addr.2.i873.idx.sink1183.i.i
  %136 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i885.i.i = icmp eq ptr %136, null
  br i1 %tobool1.not.i885.i.i, label %if.else.i898.i.i, label %if.then2.i886.i.i

if.then2.i886.i.i:                                ; preds = %if.end.i884.i.i
  %137 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %138 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i889.i.i = icmp ult i64 %137, %138
  br i1 %cmp.i889.i.i, label %if.then3.i893.i.i, label %if.end11.i890.i.i

if.then3.i893.i.i:                                ; preds = %if.then2.i886.i.i
  %139 = xor i64 %137, -1
  %sub6.i894.i.i = add i64 %138, %139
  %cond.i895.i.i = call i64 @llvm.umin.i64(i64 %sub6.i894.i.i, i64 %gepdiff.i547.i)
  %add.ptr.i896.i.i = getelementptr inbounds i8, ptr %136, i64 %137
  %call.i897.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i896.i.i, ptr noundef nonnull %s.1.ptr.i.i, i64 noundef %cond.i895.i.i) #12
  br label %if.end11.i890.i.i

if.else.i898.i.i:                                 ; preds = %if.end.i884.i.i
  call void @__msil_write(ptr noundef nonnull %s.1.ptr.i.i, i64 noundef %gepdiff.i547.i) #12
  br label %if.end11.i890.i.i

if.end11.i890.i.i:                                ; preds = %if.else.i898.i.i, %if.then3.i893.i.i, %if.then2.i886.i.i
  %140 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i892.i.i = add i64 %140, %gepdiff.i547.i
  store i64 %add.i892.i.i, ptr %len.i.i, align 8, !tbaa !38
  %incdec.ptr445.i.i = getelementptr inbounds i32, ptr %d.31152.i.i, i64 1
  %cmp413.not.i.i = icmp ugt ptr %incdec.ptr445.i.i, %r.0108310891095.i.i
  br i1 %cmp413.not.i.i, label %for.end446.i.i, label %for.body415.i.i, !llvm.loop !72

for.end446.i.i:                                   ; preds = %if.end11.i890.i.i
  %brmerge.not.i.i = icmp eq i32 %119, 0
  br i1 %brmerge.not.i.i, label %if.end452.i.i, label %if.then451.i.i

if.then451.i.i:                                   ; preds = %for.end446.i.i
  %141 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i901.i.i = icmp eq ptr %141, null
  br i1 %tobool1.not.i901.i.i, label %if.else.i914.i.i, label %if.then2.i902.i.i

if.then2.i902.i.i:                                ; preds = %if.then451.i.i
  %142 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i905.i.i = icmp ult i64 %add.i892.i.i, %142
  br i1 %cmp.i905.i.i, label %if.then3.i909.i.i, label %emit.exit915.i.i

if.then3.i909.i.i:                                ; preds = %if.then2.i902.i.i
  %.neg1113.i.i = add nuw i64 %add.i892.i.i, 1
  %143 = icmp ne i64 %142, %.neg1113.i.i
  %cond.i911.i.i = zext i1 %143 to i64
  %add.ptr.i912.i.i = getelementptr inbounds i8, ptr %141, i64 %add.i892.i.i
  %call.i913.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i912.i.i, ptr noundef nonnull @"??_C@_01LFCBOECM@?4?$AA@", i64 noundef %cond.i911.i.i) #12
  br label %emit.exit915.i.i

if.else.i914.i.i:                                 ; preds = %if.then451.i.i
  call void @__msil_write(ptr noundef nonnull @"??_C@_01LFCBOECM@?4?$AA@", i64 noundef 1) #12
  br label %emit.exit915.i.i

emit.exit915.i.i:                                 ; preds = %if.else.i914.i.i, %if.then3.i909.i.i, %if.then2.i902.i.i
  %144 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i908.i.i = add i64 %144, 1
  store i64 %add.i908.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %if.end452.i.i

if.end452.i.i:                                    ; preds = %emit.exit915.i.i, %for.end446.i.i
  %cmp4541156.i.i = icmp ult ptr %incdec.ptr445.i.i, %z.1.i.i
  %cmp4571157.i.i = icmp sgt i32 %p.addr.2.i.i, 0
  %145 = and i1 %cmp4571157.i.i, %cmp4541156.i.i
  br i1 %145, label %for.body461.i.i, label %for.end483.i.i

for.body461.i.i:                                  ; preds = %emit.exit952.i.i, %if.end452.i.i
  %p.addr.31159.i.i = phi i32 [ %sub482.i.i, %emit.exit952.i.i ], [ %p.addr.2.i.i, %if.end452.i.i ]
  %d.41158.i.i = phi ptr [ %incdec.ptr481.i.i, %emit.exit952.i.i ], [ %incdec.ptr445.i.i, %if.end452.i.i ]
  %146 = load i32, ptr %d.41158.i.i, align 4, !tbaa !24
  %cmp330.i917.i.i = icmp ugt i32 %146, 9
  br i1 %cmp330.i917.i.i, label %for.body5.i927.i.i, label %for.end12.i918.i.i

for.body5.i927.i.i:                               ; preds = %for.body5.i927.i.i, %for.body461.i.i
  %y.032.i928.i.i = phi i32 [ %div11.i933.i.i, %for.body5.i927.i.i ], [ %146, %for.body461.i.i ]
  %s.addr.131.i929.i.i = phi ptr [ %incdec.ptr9.i932.i.i, %for.body5.i927.i.i ], [ %add.ptr501.ptr.i.i, %for.body461.i.i ]
  %rem6.i930.i.i = urem i32 %y.032.i928.i.i, 10
  %147 = trunc i32 %rem6.i930.i.i to i8
  %conv8.i931.i.i = or disjoint i8 %147, 48
  %incdec.ptr9.i932.i.i = getelementptr inbounds i8, ptr %s.addr.131.i929.i.i, i64 -1
  store i8 %conv8.i931.i.i, ptr %incdec.ptr9.i932.i.i, align 1, !tbaa !4
  %div11.i933.i.i = udiv i32 %y.032.i928.i.i, 10
  %cmp3.i934.i.i = icmp ugt i32 %y.032.i928.i.i, 99
  br i1 %cmp3.i934.i.i, label %for.body5.i927.i.i, label %for.end12.i918.i.i, !llvm.loop !51

for.end12.i918.i.i:                               ; preds = %for.body5.i927.i.i, %for.body461.i.i
  %s.addr.1.lcssa.i919.i.i = phi ptr [ %add.ptr501.ptr.i.i, %for.body461.i.i ], [ %incdec.ptr9.i932.i.i, %for.body5.i927.i.i ]
  %y.0.lcssa.i920.i.i = phi i32 [ %146, %for.body461.i.i ], [ %div11.i933.i.i, %for.body5.i927.i.i ]
  %tobool.not.i921.i.i = icmp eq i32 %y.0.lcssa.i920.i.i, 0
  br i1 %tobool.not.i921.i.i, label %fmt_u.exit935.i.i, label %if.then.i922.i.i

if.then.i922.i.i:                                 ; preds = %for.end12.i918.i.i
  %148 = trunc i32 %y.0.lcssa.i920.i.i to i8
  %conv14.i923.i.i = or disjoint i8 %148, 48
  %incdec.ptr15.i924.i.i = getelementptr inbounds i8, ptr %s.addr.1.lcssa.i919.i.i, i64 -1
  store i8 %conv14.i923.i.i, ptr %incdec.ptr15.i924.i.i, align 1, !tbaa !4
  br label %fmt_u.exit935.i.i

fmt_u.exit935.i.i:                                ; preds = %if.then.i922.i.i, %for.end12.i918.i.i
  %s.addr.2.i926.i.i = phi ptr [ %incdec.ptr15.i924.i.i, %if.then.i922.i.i ], [ %s.addr.1.lcssa.i919.i.i, %for.end12.i918.i.i ]
  %cmp4681153.i.i = icmp ugt ptr %s.addr.2.i926.i.i, %buf.i524.i
  br i1 %cmp4681153.i.i, label %while.body470.i.i.preheader, label %if.end.i937.i.i

while.body470.i.i.preheader:                      ; preds = %fmt_u.exit935.i.i
  %s.addr.2.i926.i.i84 = ptrtoint ptr %s.addr.2.i926.i.i to i64
  %xtraiter85 = and i64 %s.addr.2.i926.i.i84, 7
  %lcmp.mod86.not = icmp eq i64 %xtraiter85, 0
  br i1 %lcmp.mod86.not, label %while.body470.i.i.prol.loopexit, label %while.body470.i.i.prol

while.body470.i.i.prol:                           ; preds = %while.body470.i.i.prol, %while.body470.i.i.preheader
  %s.21154.i.i.prol = phi ptr [ %incdec.ptr471.i.i.prol, %while.body470.i.i.prol ], [ %s.addr.2.i926.i.i, %while.body470.i.i.preheader ]
  %prol.iter87 = phi i64 [ %prol.iter87.next, %while.body470.i.i.prol ], [ 0, %while.body470.i.i.preheader ]
  %incdec.ptr471.i.i.prol = getelementptr inbounds i8, ptr %s.21154.i.i.prol, i64 -1
  store i8 48, ptr %incdec.ptr471.i.i.prol, align 1, !tbaa !4
  %prol.iter87.next = add i64 %prol.iter87, 1
  %prol.iter87.cmp.not = icmp eq i64 %prol.iter87.next, %xtraiter85
  br i1 %prol.iter87.cmp.not, label %while.body470.i.i.prol.loopexit, label %while.body470.i.i.prol, !llvm.loop !73

while.body470.i.i.prol.loopexit:                  ; preds = %while.body470.i.i.prol, %while.body470.i.i.preheader
  %s.21154.i.i.unr = phi ptr [ %s.addr.2.i926.i.i, %while.body470.i.i.preheader ], [ %incdec.ptr471.i.i.prol, %while.body470.i.i.prol ]
  %incdec.ptr471.i.i.lcssa.unr = phi ptr [ undef, %while.body470.i.i.preheader ], [ %incdec.ptr471.i.i.prol, %while.body470.i.i.prol ]
  %149 = sub i64 %buf.i524.i76, %s.addr.2.i926.i.i84
  %150 = icmp ugt i64 %149, -8
  br i1 %150, label %if.end.i937.i.i, label %while.body470.i.i

while.body470.i.i:                                ; preds = %while.body470.i.i, %while.body470.i.i.prol.loopexit
  %s.21154.i.i = phi ptr [ %incdec.ptr471.i.i.7, %while.body470.i.i ], [ %s.21154.i.i.unr, %while.body470.i.i.prol.loopexit ]
  %incdec.ptr471.i.i = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -1
  store i8 48, ptr %incdec.ptr471.i.i, align 1, !tbaa !4
  %incdec.ptr471.i.i.1 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -2
  store i8 48, ptr %incdec.ptr471.i.i.1, align 1, !tbaa !4
  %incdec.ptr471.i.i.2 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -3
  store i8 48, ptr %incdec.ptr471.i.i.2, align 1, !tbaa !4
  %incdec.ptr471.i.i.3 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -4
  store i8 48, ptr %incdec.ptr471.i.i.3, align 1, !tbaa !4
  %incdec.ptr471.i.i.4 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -5
  store i8 48, ptr %incdec.ptr471.i.i.4, align 1, !tbaa !4
  %incdec.ptr471.i.i.5 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -6
  store i8 48, ptr %incdec.ptr471.i.i.5, align 1, !tbaa !4
  %incdec.ptr471.i.i.6 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -7
  store i8 48, ptr %incdec.ptr471.i.i.6, align 1, !tbaa !4
  %incdec.ptr471.i.i.7 = getelementptr inbounds i8, ptr %s.21154.i.i, i64 -8
  store i8 48, ptr %incdec.ptr471.i.i.7, align 1, !tbaa !4
  %cmp468.i.i.7 = icmp ugt ptr %incdec.ptr471.i.i.7, %buf.i524.i
  br i1 %cmp468.i.i.7, label %while.body470.i.i, label %if.end.i937.i.i, !llvm.loop !74

if.end.i937.i.i:                                  ; preds = %while.body470.i.i, %while.body470.i.i.prol.loopexit, %fmt_u.exit935.i.i
  %s.2.lcssa.i.i = phi ptr [ %s.addr.2.i926.i.i, %fmt_u.exit935.i.i ], [ %incdec.ptr471.i.i.lcssa.unr, %while.body470.i.i.prol.loopexit ], [ %incdec.ptr471.i.i.7, %while.body470.i.i ]
  %cond478.i.i = call i32 @llvm.smin.i32(i32 %p.addr.31159.i.i, i32 9)
  %conv479.i.i = zext nneg i32 %cond478.i.i to i64
  %151 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i938.i.i = icmp eq ptr %151, null
  br i1 %tobool1.not.i938.i.i, label %if.else.i951.i.i, label %if.then2.i939.i.i

if.then2.i939.i.i:                                ; preds = %if.end.i937.i.i
  %152 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %153 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i942.i.i = icmp ult i64 %152, %153
  br i1 %cmp.i942.i.i, label %if.then3.i946.i.i, label %emit.exit952.i.i

if.then3.i946.i.i:                                ; preds = %if.then2.i939.i.i
  %154 = xor i64 %152, -1
  %sub6.i947.i.i = add i64 %153, %154
  %cond.i948.i.i = call i64 @llvm.umin.i64(i64 %sub6.i947.i.i, i64 %conv479.i.i)
  %add.ptr.i949.i.i = getelementptr inbounds i8, ptr %151, i64 %152
  %call.i950.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i949.i.i, ptr noundef nonnull %s.2.lcssa.i.i, i64 noundef %cond.i948.i.i) #12
  br label %emit.exit952.i.i

if.else.i951.i.i:                                 ; preds = %if.end.i937.i.i
  call void @__msil_write(ptr noundef nonnull %s.2.lcssa.i.i, i64 noundef %conv479.i.i) #12
  br label %emit.exit952.i.i

emit.exit952.i.i:                                 ; preds = %if.else.i951.i.i, %if.then3.i946.i.i, %if.then2.i939.i.i
  %155 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i945.i.i = add i64 %155, %conv479.i.i
  store i64 %add.i945.i.i, ptr %len.i.i, align 8, !tbaa !38
  %incdec.ptr481.i.i = getelementptr inbounds i32, ptr %d.41158.i.i, i64 1
  %sub482.i.i = add nsw i32 %p.addr.31159.i.i, -9
  %cmp454.i.i = icmp ult ptr %incdec.ptr481.i.i, %z.1.i.i
  %cmp457.i.i = icmp sgt i32 %p.addr.31159.i.i, 9
  %156 = select i1 %cmp454.i.i, i1 %cmp457.i.i, i1 false
  br i1 %156, label %for.body461.i.i, label %for.end483.i.i, !llvm.loop !75

for.end483.i.i:                                   ; preds = %emit.exit952.i.i, %if.end452.i.i
  %p.addr.3.lcssa.i.i = phi i32 [ %p.addr.2.i.i, %if.end452.i.i ], [ %sub482.i.i, %emit.exit952.i.i ]
  %add484.i.i = add nsw i32 %p.addr.3.lcssa.i.i, 9
  call fastcc void @pad(ptr noundef nonnull %o, i8 noundef 48, i32 noundef %add484.i.i, i32 noundef 9, i32 noundef 0) #14
  br label %if.end563.i.i

if.else485.i.i:                                   ; preds = %emit.exit862.i.i
  %add.ptr489.i.i = getelementptr inbounds i32, ptr %a.4.i.i, i64 1
  %spec.select1112.i.i = select i1 %cmp178.i.i, ptr %z.1.i.i, ptr %add.ptr489.i.i
  %cmp4951145.i.i = icmp sgt i32 %p.addr.2.i.i, -1
  br i1 %cmp4951145.i.i, label %for.body499.i.i, label %for.end558.i.i

for.body499.i.i:                                  ; preds = %emit.exit1021.i.i, %if.else485.i.i
  %p.addr.41147.i.i = phi i32 [ %sub555.i.i, %emit.exit1021.i.i ], [ %p.addr.2.i.i, %if.else485.i.i ]
  %d.51146.i.i = phi ptr [ %incdec.ptr557.i.i, %emit.exit1021.i.i ], [ %a.4.i.i, %if.else485.i.i ]
  %157 = load i32, ptr %d.51146.i.i, align 4, !tbaa !24
  %cmp330.i954.i.i = icmp ugt i32 %157, 9
  br i1 %cmp330.i954.i.i, label %for.body5.i964.i.i, label %for.end12.i955.i.i

for.body5.i964.i.i:                               ; preds = %for.body5.i964.i.i, %for.body499.i.i
  %y.032.i965.i.i = phi i32 [ %div11.i970.i.i, %for.body5.i964.i.i ], [ %157, %for.body499.i.i ]
  %s.addr.131.i966.idx.i.i = phi i64 [ %s.addr.131.i966.add.i.i, %for.body5.i964.i.i ], [ 9, %for.body499.i.i ]
  %rem6.i967.i.i = urem i32 %y.032.i965.i.i, 10
  %158 = trunc i32 %rem6.i967.i.i to i8
  %conv8.i968.i.i = or disjoint i8 %158, 48
  %s.addr.131.i966.add.i.i = add nsw i64 %s.addr.131.i966.idx.i.i, -1
  %incdec.ptr9.i969.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.addr.131.i966.add.i.i
  store i8 %conv8.i968.i.i, ptr %incdec.ptr9.i969.ptr.i.i, align 1, !tbaa !4
  %div11.i970.i.i = udiv i32 %y.032.i965.i.i, 10
  %cmp3.i971.i.i = icmp ugt i32 %y.032.i965.i.i, 99
  br i1 %cmp3.i971.i.i, label %for.body5.i964.i.i, label %for.end12.i955.i.i, !llvm.loop !51

for.end12.i955.i.i:                               ; preds = %for.body5.i964.i.i, %for.body499.i.i
  %s.addr.1.lcssa.i956.idx.i.i = phi i64 [ 9, %for.body499.i.i ], [ %s.addr.131.i966.add.i.i, %for.body5.i964.i.i ]
  %y.0.lcssa.i957.i.i = phi i32 [ %157, %for.body499.i.i ], [ %div11.i970.i.i, %for.body5.i964.i.i ]
  %tobool.not.i958.i.i = icmp eq i32 %y.0.lcssa.i957.i.i, 0
  br i1 %tobool.not.i958.i.i, label %fmt_u.exit972.i.i, label %if.then.i959.i.i

if.then.i959.i.i:                                 ; preds = %for.end12.i955.i.i
  %159 = trunc i32 %y.0.lcssa.i957.i.i to i8
  %conv14.i960.i.i = or disjoint i8 %159, 48
  %s.addr.1.lcssa.i956.add.i.i = add nsw i64 %s.addr.1.lcssa.i956.idx.i.i, -1
  %incdec.ptr15.i961.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.addr.1.lcssa.i956.add.i.i
  store i8 %conv14.i960.i.i, ptr %incdec.ptr15.i961.ptr.i.i, align 1, !tbaa !4
  br label %fmt_u.exit972.i.i

fmt_u.exit972.i.i:                                ; preds = %if.then.i959.i.i, %for.end12.i955.i.i
  %s.addr.2.i963.idx.i.i = phi i64 [ %s.addr.1.lcssa.i956.add.i.i, %if.then.i959.i.i ], [ %s.addr.1.lcssa.i956.idx.i.i, %for.end12.i955.i.i ]
  %s.addr.2.i963.ptr.i.i = getelementptr inbounds i8, ptr %buf.i524.i, i64 %s.addr.2.i963.idx.i.i
  %cmp506.i.i = icmp eq i64 %s.addr.2.i963.idx.i.i, 9
  br i1 %cmp506.i.i, label %if.then508.i.i, label %if.end510.i.i

if.then508.i.i:                                   ; preds = %fmt_u.exit972.i.i
  %incdec.ptr509.i.i = getelementptr inbounds i8, ptr %s.addr.2.i963.ptr.i.i, i64 -1
  store i8 48, ptr %incdec.ptr509.i.i, align 1, !tbaa !4
  br label %if.end510.i.i

if.end510.i.i:                                    ; preds = %if.then508.i.i, %fmt_u.exit972.i.i
  %s.3.i.i = phi ptr [ %incdec.ptr509.i.i, %if.then508.i.i ], [ %s.addr.2.i963.ptr.i.i, %fmt_u.exit972.i.i ]
  %s.3.i.i77 = ptrtoint ptr %s.3.i.i to i64
  %cmp511.not.i.i = icmp eq ptr %d.51146.i.i, %a.4.i.i
  br i1 %cmp511.not.i.i, label %if.else521.i.i, label %while.cond514.preheader.i.i

while.cond514.preheader.i.i:                      ; preds = %if.end510.i.i
  %cmp5161141.i.i = icmp ugt ptr %s.3.i.i, %buf.i524.i
  br i1 %cmp5161141.i.i, label %while.body518.i.i.preheader, label %if.end530.i.i

while.body518.i.i.preheader:                      ; preds = %while.cond514.preheader.i.i
  %xtraiter78 = and i64 %s.3.i.i77, 7
  %lcmp.mod79.not = icmp eq i64 %xtraiter78, 0
  br i1 %lcmp.mod79.not, label %while.body518.i.i.prol.loopexit, label %while.body518.i.i.prol

while.body518.i.i.prol:                           ; preds = %while.body518.i.i.prol, %while.body518.i.i.preheader
  %s.41142.i.i.prol = phi ptr [ %incdec.ptr519.i.i.prol, %while.body518.i.i.prol ], [ %s.3.i.i, %while.body518.i.i.preheader ]
  %prol.iter80 = phi i64 [ %prol.iter80.next, %while.body518.i.i.prol ], [ 0, %while.body518.i.i.preheader ]
  %incdec.ptr519.i.i.prol = getelementptr inbounds i8, ptr %s.41142.i.i.prol, i64 -1
  store i8 48, ptr %incdec.ptr519.i.i.prol, align 1, !tbaa !4
  %prol.iter80.next = add i64 %prol.iter80, 1
  %prol.iter80.cmp.not = icmp eq i64 %prol.iter80.next, %xtraiter78
  br i1 %prol.iter80.cmp.not, label %while.body518.i.i.prol.loopexit, label %while.body518.i.i.prol, !llvm.loop !76

while.body518.i.i.prol.loopexit:                  ; preds = %while.body518.i.i.prol, %while.body518.i.i.preheader
  %s.41142.i.i.unr = phi ptr [ %s.3.i.i, %while.body518.i.i.preheader ], [ %incdec.ptr519.i.i.prol, %while.body518.i.i.prol ]
  %incdec.ptr519.i.i.lcssa.unr = phi ptr [ undef, %while.body518.i.i.preheader ], [ %incdec.ptr519.i.i.prol, %while.body518.i.i.prol ]
  %160 = sub i64 %buf.i524.i76, %s.3.i.i77
  %161 = icmp ugt i64 %160, -8
  br i1 %161, label %if.end530.i.i, label %while.body518.i.i

while.body518.i.i:                                ; preds = %while.body518.i.i, %while.body518.i.i.prol.loopexit
  %s.41142.i.i = phi ptr [ %incdec.ptr519.i.i.7, %while.body518.i.i ], [ %s.41142.i.i.unr, %while.body518.i.i.prol.loopexit ]
  %incdec.ptr519.i.i = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -1
  store i8 48, ptr %incdec.ptr519.i.i, align 1, !tbaa !4
  %incdec.ptr519.i.i.1 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -2
  store i8 48, ptr %incdec.ptr519.i.i.1, align 1, !tbaa !4
  %incdec.ptr519.i.i.2 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -3
  store i8 48, ptr %incdec.ptr519.i.i.2, align 1, !tbaa !4
  %incdec.ptr519.i.i.3 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -4
  store i8 48, ptr %incdec.ptr519.i.i.3, align 1, !tbaa !4
  %incdec.ptr519.i.i.4 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -5
  store i8 48, ptr %incdec.ptr519.i.i.4, align 1, !tbaa !4
  %incdec.ptr519.i.i.5 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -6
  store i8 48, ptr %incdec.ptr519.i.i.5, align 1, !tbaa !4
  %incdec.ptr519.i.i.6 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -7
  store i8 48, ptr %incdec.ptr519.i.i.6, align 1, !tbaa !4
  %incdec.ptr519.i.i.7 = getelementptr inbounds i8, ptr %s.41142.i.i, i64 -8
  store i8 48, ptr %incdec.ptr519.i.i.7, align 1, !tbaa !4
  %cmp516.i.i.7 = icmp ugt ptr %incdec.ptr519.i.i.7, %buf.i524.i
  br i1 %cmp516.i.i.7, label %while.body518.i.i, label %if.end530.i.i, !llvm.loop !77

if.else521.i.i:                                   ; preds = %if.end510.i.i
  %incdec.ptr522.i.i = getelementptr inbounds i8, ptr %s.3.i.i, i64 1
  %162 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i974.i.i = icmp eq ptr %162, null
  br i1 %tobool1.not.i974.i.i, label %if.else.i987.i.i, label %if.then2.i975.i.i

if.then2.i975.i.i:                                ; preds = %if.else521.i.i
  %163 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %164 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i978.i.i = icmp ult i64 %163, %164
  br i1 %cmp.i978.i.i, label %if.then3.i982.i.i, label %emit.exit988.i.i

if.then3.i982.i.i:                                ; preds = %if.then2.i975.i.i
  %.neg.i546.i = add nuw i64 %163, 1
  %165 = icmp ne i64 %164, %.neg.i546.i
  %cond.i984.i.i = zext i1 %165 to i64
  %add.ptr.i985.i.i = getelementptr inbounds i8, ptr %162, i64 %163
  %call.i986.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i985.i.i, ptr noundef nonnull %s.3.i.i, i64 noundef %cond.i984.i.i) #12
  br label %emit.exit988.i.i

if.else.i987.i.i:                                 ; preds = %if.else521.i.i
  call void @__msil_write(ptr noundef nonnull %s.3.i.i, i64 noundef 1) #12
  br label %emit.exit988.i.i

emit.exit988.i.i:                                 ; preds = %if.else.i987.i.i, %if.then3.i982.i.i, %if.then2.i975.i.i
  %166 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i981.i.i = add i64 %166, 1
  store i64 %add.i981.i.i, ptr %len.i.i, align 8, !tbaa !38
  %cmp523.i.i = icmp sgt i32 %p.addr.41147.i.i, 0
  %brmerge808.i.i = or i1 %tobool344.not.i.i, %cmp523.i.i
  br i1 %brmerge808.i.i, label %if.then528.i.i, label %if.end530.i.i

if.then528.i.i:                                   ; preds = %emit.exit988.i.i
  %167 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i990.i.i = icmp eq ptr %167, null
  br i1 %tobool1.not.i990.i.i, label %if.else.i1003.i.i, label %if.then2.i991.i.i

if.then2.i991.i.i:                                ; preds = %if.then528.i.i
  %168 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i994.i.i = icmp ult i64 %add.i981.i.i, %168
  br i1 %cmp.i994.i.i, label %if.then3.i998.i.i, label %emit.exit1004.i.i

if.then3.i998.i.i:                                ; preds = %if.then2.i991.i.i
  %reass.sub.i.i = sub i64 %168, %166
  %169 = icmp ne i64 %reass.sub.i.i, 2
  %cond.i1000.i.i = zext i1 %169 to i64
  %add.ptr.i1001.i.i = getelementptr inbounds i8, ptr %167, i64 %add.i981.i.i
  %call.i1002.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i1001.i.i, ptr noundef nonnull @"??_C@_01LFCBOECM@?4?$AA@", i64 noundef %cond.i1000.i.i) #12
  br label %emit.exit1004.i.i

if.else.i1003.i.i:                                ; preds = %if.then528.i.i
  call void @__msil_write(ptr noundef nonnull @"??_C@_01LFCBOECM@?4?$AA@", i64 noundef 1) #12
  br label %emit.exit1004.i.i

emit.exit1004.i.i:                                ; preds = %if.else.i1003.i.i, %if.then3.i998.i.i, %if.then2.i991.i.i
  %170 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i997.i.i = add i64 %170, 1
  store i64 %add.i997.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %if.end530.i.i

if.end530.i.i:                                    ; preds = %emit.exit1004.i.i, %emit.exit988.i.i, %while.body518.i.i, %while.body518.i.i.prol.loopexit, %while.cond514.preheader.i.i
  %s.5.i.i = phi ptr [ %incdec.ptr522.i.i, %emit.exit1004.i.i ], [ %incdec.ptr522.i.i, %emit.exit988.i.i ], [ %s.3.i.i, %while.cond514.preheader.i.i ], [ %incdec.ptr519.i.i.lcssa.unr, %while.body518.i.i.prol.loopexit ], [ %incdec.ptr519.i.i.7, %while.body518.i.i ]
  %sub.ptr.rhs.cast534.i.i = ptrtoint ptr %s.5.i.i to i64
  %sub.ptr.sub535.i.i = sub i64 %sub.ptr.lhs.cast533.i.i, %sub.ptr.rhs.cast534.i.i
  %conv536.i.i = zext nneg i32 %p.addr.41147.i.i to i64
  %cond548.i.i = call i64 @llvm.smin.i64(i64 %sub.ptr.sub535.i.i, i64 %conv536.i.i)
  %tobool.not.i1005.i.i = icmp eq i64 %cond548.i.i, 0
  br i1 %tobool.not.i1005.i.i, label %emit.exit1021.i.i, label %if.end.i1006.i.i

if.end.i1006.i.i:                                 ; preds = %if.end530.i.i
  %171 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i1007.i.i = icmp eq ptr %171, null
  br i1 %tobool1.not.i1007.i.i, label %if.else.i1020.i.i, label %if.then2.i1008.i.i

if.then2.i1008.i.i:                               ; preds = %if.end.i1006.i.i
  %172 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %173 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i1011.i.i = icmp ult i64 %172, %173
  br i1 %cmp.i1011.i.i, label %if.then3.i1015.i.i, label %if.end11.i1012.i.i

if.then3.i1015.i.i:                               ; preds = %if.then2.i1008.i.i
  %174 = xor i64 %172, -1
  %sub6.i1016.i.i = add i64 %173, %174
  %cond.i1017.i.i = call i64 @llvm.umin.i64(i64 %sub6.i1016.i.i, i64 %cond548.i.i)
  %add.ptr.i1018.i.i = getelementptr inbounds i8, ptr %171, i64 %172
  %call.i1019.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i1018.i.i, ptr noundef nonnull %s.5.i.i, i64 noundef %cond.i1017.i.i) #12
  br label %if.end11.i1012.i.i

if.else.i1020.i.i:                                ; preds = %if.end.i1006.i.i
  call void @__msil_write(ptr noundef nonnull %s.5.i.i, i64 noundef %cond548.i.i) #12
  br label %if.end11.i1012.i.i

if.end11.i1012.i.i:                               ; preds = %if.else.i1020.i.i, %if.then3.i1015.i.i, %if.then2.i1008.i.i
  %175 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i1014.i.i = add i64 %175, %cond548.i.i
  store i64 %add.i1014.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %emit.exit1021.i.i

emit.exit1021.i.i:                                ; preds = %if.end11.i1012.i.i, %if.end530.i.i
  %conv554.i.i = trunc i64 %sub.ptr.sub535.i.i to i32
  %sub555.i.i = sub nsw i32 %p.addr.41147.i.i, %conv554.i.i
  %incdec.ptr557.i.i = getelementptr inbounds i32, ptr %d.51146.i.i, i64 1
  %cmp492.i.i = icmp ult ptr %incdec.ptr557.i.i, %spec.select1112.i.i
  %cmp495.i.i = icmp sgt i32 %sub555.i.i, -1
  %176 = select i1 %cmp492.i.i, i1 %cmp495.i.i, i1 false
  br i1 %176, label %for.body499.i.i, label %for.end558.i.i, !llvm.loop !78

for.end558.i.i:                                   ; preds = %emit.exit1021.i.i, %if.else485.i.i
  %p.addr.4.lcssa.i.i = phi i32 [ %p.addr.2.i.i, %if.else485.i.i ], [ %sub555.i.i, %emit.exit1021.i.i ]
  %add559.i.i = add nsw i32 %p.addr.4.lcssa.i.i, 18
  call fastcc void @pad(ptr noundef %o, i8 noundef 48, i32 noundef %add559.i.i, i32 noundef 18, i32 noundef 0) #14
  %sub.ptr.rhs.cast561.i.i = ptrtoint ptr %estr.1.i.i to i64
  %sub.ptr.sub562.i.i = sub i64 %sub.ptr.lhs.cast379.i.i, %sub.ptr.rhs.cast561.i.i
  %tobool.not.i1022.i.i = icmp eq ptr %add.ptr.i526.i, %estr.1.i.i
  br i1 %tobool.not.i1022.i.i, label %if.end563.i.i, label %if.end.i1023.i.i

if.end.i1023.i.i:                                 ; preds = %for.end558.i.i
  %177 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i1024.i.i = icmp eq ptr %177, null
  br i1 %tobool1.not.i1024.i.i, label %if.else.i1037.i.i, label %if.then2.i1025.i.i

if.then2.i1025.i.i:                               ; preds = %if.end.i1023.i.i
  %178 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %179 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i1028.i.i = icmp ult i64 %178, %179
  br i1 %cmp.i1028.i.i, label %if.then3.i1032.i.i, label %if.end11.i1029.i.i

if.then3.i1032.i.i:                               ; preds = %if.then2.i1025.i.i
  %180 = xor i64 %178, -1
  %sub6.i1033.i.i = add i64 %179, %180
  %cond.i1034.i.i = call i64 @llvm.umin.i64(i64 %sub6.i1033.i.i, i64 %sub.ptr.sub562.i.i)
  %add.ptr.i1035.i.i = getelementptr inbounds i8, ptr %177, i64 %178
  %call.i1036.i.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i1035.i.i, ptr noundef %estr.1.i.i, i64 noundef %cond.i1034.i.i) #12
  br label %if.end11.i1029.i.i

if.else.i1037.i.i:                                ; preds = %if.end.i1023.i.i
  call void @__msil_write(ptr noundef %estr.1.i.i, i64 noundef %sub.ptr.sub562.i.i) #12
  br label %if.end11.i1029.i.i

if.end11.i1029.i.i:                               ; preds = %if.else.i1037.i.i, %if.then3.i1032.i.i, %if.then2.i1025.i.i
  %181 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i1031.i.i = add i64 %181, %sub.ptr.sub562.i.i
  store i64 %add.i1031.i.i, ptr %len.i.i, align 8, !tbaa !38
  br label %if.end563.i.i

if.end563.i.i:                                    ; preds = %if.end11.i1029.i.i, %for.end558.i.i, %for.end483.i.i
  %xor564.i.i = xor i32 %fl.2.i, 8192
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %w.0.i, i32 noundef %add400.i.i, i32 noundef %xor564.i.i) #14
  %cond573.i.i = call i32 @llvm.smax.i32(i32 %add400.i.i, i32 %w.0.i)
  br label %fmt_fp.exit.i

fmt_fp.exit.i:                                    ; preds = %if.end563.i.i, %if.end341.i.i, %emit.exit824.i.i
  %retval.0.i545.i = phi i32 [ %cond30.i.i, %emit.exit824.i.i ], [ %cond573.i.i, %if.end563.i.i ], [ -1, %if.end341.i.i ]
  call void @llvm.lifetime.end.p0(i64 16, ptr nonnull %ebuf.i525.i) #13
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %buf.i524.i) #13
  call void @llvm.lifetime.end.p0(i64 576, ptr nonnull %big.i.i) #13
  br label %cond.end276.i

cond.end276.i:                                    ; preds = %fmt_fp.exit.i, %fmt_hexfloat.exit.i
  %cond277.i = phi i32 [ %cond36.i.i, %fmt_hexfloat.exit.i ], [ %retval.0.i545.i, %fmt_fp.exit.i ]
  %add278.i = add nsw i32 %cond277.i, %l.0675.i
  br label %for.cond.outer.backedge.i

for.cond.outer.backedge.i:                        ; preds = %emit.exit625.i, %cond.end276.i, %sw.bb140.i, %sw.bb138.i, %sw.bb136.i, %sw.bb134.i, %sw.bb132.i, %sw.bb131.i, %sw.bb130.i, %sw.bb.i
  %arg.sroa.0.0.ph.be.i = phi i64 [ %arg.sroa.0.5.i, %emit.exit625.i ], [ %arg.sroa.0.2.i, %cond.end276.i ], [ %arg.sroa.0.2.i, %sw.bb.i ], [ %arg.sroa.0.2.i, %sw.bb140.i ], [ %arg.sroa.0.2.i, %sw.bb138.i ], [ %arg.sroa.0.2.i, %sw.bb136.i ], [ %arg.sroa.0.2.i, %sw.bb134.i ], [ %arg.sroa.0.2.i, %sw.bb132.i ], [ %arg.sroa.0.2.i, %sw.bb131.i ], [ %arg.sroa.0.2.i, %sw.bb130.i ]
  %l.0.ph.be.i = phi i32 [ %add316.i, %emit.exit625.i ], [ %add278.i, %cond.end276.i ], [ %l.0675.i, %sw.bb.i ], [ %l.0675.i, %sw.bb140.i ], [ %l.0675.i, %sw.bb138.i ], [ %l.0675.i, %sw.bb136.i ], [ %l.0675.i, %sw.bb134.i ], [ %l.0675.i, %sw.bb132.i ], [ %l.0675.i, %sw.bb131.i ], [ %l.0675.i, %sw.bb130.i ]
  %182 = load i8, ptr %incdec.ptr98.i, align 1, !tbaa !4
  %tobool.not673.i = icmp eq i8 %182, 0
  br i1 %tobool.not673.i, label %printf_core.exit, label %for.cond1.preheader.lr.ph.i

sw.epilog280.i:                                   ; preds = %if.end256.i, %sw.bb231.i, %if.end210.i, %if.end201.i
  %sub.ptr.lhs.cast282.pre-phi.i = phi i64 [ %sub.ptr.lhs.cast162.i, %if.end201.i ], [ %.pre.i, %if.end256.i ], [ %sub.ptr.lhs.cast162.i, %sw.bb231.i ], [ %sub.ptr.lhs.cast162.i, %if.end210.i ]
  %arg.sroa.0.5.i = phi i64 [ 0, %if.end201.i ], [ %arg.sroa.0.2.i, %if.end256.i ], [ %arg.sroa.0.2.i, %sw.bb231.i ], [ %arg.sroa.0.4.i, %if.end210.i ]
  %a.1.i = phi ptr [ %add.ptr114.i, %if.end201.i ], [ %cond240.i, %if.end256.i ], [ %add.ptr233.i, %sw.bb231.i ], [ %a.0.i, %if.end210.i ]
  %z.1.i = phi ptr [ %add.ptr114.i, %if.end201.i ], [ %add.ptr249.i, %if.end256.i ], [ %add.ptr114.i, %sw.bb231.i ], [ %add.ptr114.i, %if.end210.i ]
  %fl.6.i = phi i32 [ %spec.select.i, %if.end201.i ], [ %and128.i, %if.end256.i ], [ %and128.i, %sw.bb231.i ], [ %spec.select.i, %if.end210.i ]
  %p.3.i = phi i32 [ 0, %if.end201.i ], [ %conv260.i, %if.end256.i ], [ 1, %sw.bb231.i ], [ %p.2.add216.i, %if.end210.i ]
  %prefix.2.i = phi ptr [ %prefix.1.i, %if.end201.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %if.end256.i ], [ @"??_C@_09PLAANEMO@?9?$CL?5?5?50X0x?$AA@", %sw.bb231.i ], [ %prefix.1.i, %if.end210.i ]
  %pl.2.i = phi i32 [ %pl.1.i, %if.end201.i ], [ 0, %if.end256.i ], [ 0, %sw.bb231.i ], [ %pl.1.i, %if.end210.i ]
  %conv281.i = sext i32 %p.3.i to i64
  %sub.ptr.rhs.cast283.i = ptrtoint ptr %a.1.i to i64
  %sub.ptr.sub284.i = sub i64 %sub.ptr.lhs.cast282.pre-phi.i, %sub.ptr.rhs.cast283.i
  %cmp285.i = icmp sgt i64 %sub.ptr.sub284.i, %conv281.i
  %conv291.i = trunc i64 %sub.ptr.sub284.i to i32
  %spec.select459.i = select i1 %cmp285.i, i32 %conv291.i, i32 %p.3.i
  %sub293.i = sub nuw nsw i32 2147483647, %pl.2.i
  %cmp294.i = icmp sgt i32 %spec.select459.i, %sub293.i
  br i1 %cmp294.i, label %printf_core.exit, label %if.end297.i

if.end297.i:                                      ; preds = %sw.epilog280.i
  %add298.i = add nsw i32 %spec.select459.i, %pl.2.i
  %spec.select460.i = call i32 @llvm.smax.i32(i32 %w.0.i, i32 %add298.i)
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %spec.select460.i, i32 noundef %add298.i, i32 noundef %fl.6.i) #14
  %conv305.i = zext nneg i32 %pl.2.i to i64
  %tobool.not.i592.i = icmp eq i32 %pl.2.i, 0
  br i1 %tobool.not.i592.i, label %emit.exit608.i, label %if.end.i593.i

if.end.i593.i:                                    ; preds = %if.end297.i
  %183 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i594.i = icmp eq ptr %183, null
  br i1 %tobool1.not.i594.i, label %if.else.i607.i, label %if.then2.i595.i

if.then2.i595.i:                                  ; preds = %if.end.i593.i
  %184 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %185 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i598.i = icmp ult i64 %184, %185
  br i1 %cmp.i598.i, label %if.then3.i602.i, label %if.end11.i599.i

if.then3.i602.i:                                  ; preds = %if.then2.i595.i
  %186 = xor i64 %184, -1
  %sub6.i603.i = add i64 %185, %186
  %cond.i604.i = call i64 @llvm.umin.i64(i64 %sub6.i603.i, i64 %conv305.i)
  %add.ptr.i605.i = getelementptr inbounds i8, ptr %183, i64 %184
  %call.i606.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i605.i, ptr noundef %prefix.2.i, i64 noundef %cond.i604.i) #12
  br label %if.end11.i599.i

if.else.i607.i:                                   ; preds = %if.end.i593.i
  call void @__msil_write(ptr noundef %prefix.2.i, i64 noundef %conv305.i) #12
  br label %if.end11.i599.i

if.end11.i599.i:                                  ; preds = %if.else.i607.i, %if.then3.i602.i, %if.then2.i595.i
  %187 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i601.i = add i64 %187, %conv305.i
  store i64 %add.i601.i, ptr %len.i.i, align 8, !tbaa !38
  br label %emit.exit608.i

emit.exit608.i:                                   ; preds = %if.end11.i599.i, %if.end297.i
  %xor.i = xor i32 %fl.6.i, 65536
  call fastcc void @pad(ptr noundef %o, i8 noundef 48, i32 noundef %spec.select460.i, i32 noundef %add298.i, i32 noundef %xor.i) #14
  call fastcc void @pad(ptr noundef %o, i8 noundef 48, i32 noundef %spec.select459.i, i32 noundef %conv291.i, i32 noundef 0) #14
  %tobool.not.i609.i = icmp eq ptr %z.1.i, %a.1.i
  br i1 %tobool.not.i609.i, label %emit.exit625.i, label %if.end.i610.i

if.end.i610.i:                                    ; preds = %emit.exit608.i
  %188 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i611.i = icmp eq ptr %188, null
  br i1 %tobool1.not.i611.i, label %if.else.i624.i, label %if.then2.i612.i

if.then2.i612.i:                                  ; preds = %if.end.i610.i
  %189 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %190 = load i64, ptr %cap.i.i, align 8, !tbaa !37
  %cmp.i615.i = icmp ult i64 %189, %190
  br i1 %cmp.i615.i, label %if.then3.i619.i, label %if.end11.i616.i

if.then3.i619.i:                                  ; preds = %if.then2.i612.i
  %191 = xor i64 %189, -1
  %sub6.i620.i = add i64 %190, %191
  %cond.i621.i = call i64 @llvm.umin.i64(i64 %sub6.i620.i, i64 %sub.ptr.sub284.i)
  %add.ptr.i622.i = getelementptr inbounds i8, ptr %188, i64 %189
  %call.i623.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i622.i, ptr noundef %a.1.i, i64 noundef %cond.i621.i) #12
  br label %if.end11.i616.i

if.else.i624.i:                                   ; preds = %if.end.i610.i
  call void @__msil_write(ptr noundef %a.1.i, i64 noundef %sub.ptr.sub284.i) #12
  br label %if.end11.i616.i

if.end11.i616.i:                                  ; preds = %if.else.i624.i, %if.then3.i619.i, %if.then2.i612.i
  %192 = load i64, ptr %len.i.i, align 8, !tbaa !38
  %add.i618.i = add i64 %192, %sub.ptr.sub284.i
  store i64 %add.i618.i, ptr %len.i.i, align 8, !tbaa !38
  br label %emit.exit625.i

emit.exit625.i:                                   ; preds = %if.end11.i616.i, %emit.exit608.i
  %xor314.i = xor i32 %fl.6.i, 8192
  call fastcc void @pad(ptr noundef %o, i8 noundef 32, i32 noundef %spec.select460.i, i32 noundef %add298.i, i32 noundef %xor314.i) #14
  %add316.i = add nsw i32 %spec.select460.i, %l.0675.i
  br label %for.cond.outer.backedge.i

printf_core.exit:                                 ; preds = %sw.epilog280.i, %for.cond.outer.backedge.i, %sw.bb262.i, %land.lhs.true252.i, %ifmt_tail.i, %if.end113.i, %do.end.i, %do.body.i, %getint.exit.i, %if.then49.i, %if.end11.i.i, %entry
  %retval.0.i = phi i32 [ 0, %entry ], [ -1, %do.body.i ], [ %add.i, %if.end11.i.i ], [ -1, %if.then49.i ], [ -1, %getint.exit.i ], [ -1, %ifmt_tail.i ], [ -1, %land.lhs.true252.i ], [ -1, %sw.bb262.i ], [ -1, %if.end113.i ], [ -1, %sw.epilog280.i ], [ -1, %do.end.i ], [ %l.0.ph.be.i, %for.cond.outer.backedge.i ]
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %buf.i) #13
  %193 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool.not = icmp eq ptr %193, null
  br i1 %tobool.not, label %if.end, label %land.lhs.true

land.lhs.true:                                    ; preds = %printf_core.exit
  %cap = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 1
  %194 = load i64, ptr %cap, align 8, !tbaa !37
  %tobool1.not = icmp eq i64 %194, 0
  br i1 %tobool1.not, label %if.end, label %if.then

if.then:                                          ; preds = %land.lhs.true
  %len = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  %195 = load i64, ptr %len, align 8, !tbaa !38
  %sub = add i64 %194, -1
  %.sub = call i64 @llvm.umin.i64(i64 %195, i64 %sub)
  %arrayidx = getelementptr inbounds i8, ptr %193, i64 %.sub
  store i8 0, ptr %arrayidx, align 1, !tbaa !4
  br label %if.end

if.end:                                           ; preds = %if.then, %land.lhs.true, %printf_core.exit
  ret i32 %retval.0.i
}

declare dso_local void @__msil_write(ptr noundef, i64 noundef) local_unnamed_addr #3

declare dso_local i64 @__msil_va_i64(ptr noundef) local_unnamed_addr #3

declare dso_local ptr @__msil_va_ptr(ptr noundef) local_unnamed_addr #3

declare dso_local double @__msil_va_f64(ptr noundef) local_unnamed_addr #3

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umin.i32(i32, i32) #7

; Function Attrs: nounwind
define internal fastcc void @pad(ptr nocapture noundef %o, i8 noundef %c, i32 noundef %w, i32 noundef %l, i32 noundef %fl) unnamed_addr #2 {
entry:
  %buf = alloca [256 x i8], align 16
  call void @llvm.lifetime.start.p0(i64 256, ptr nonnull %buf) #13
  %and = and i32 %fl, 73728
  %tobool.not = icmp eq i32 %and, 0
  %cmp.not = icmp slt i32 %l, %w
  %or.cond = and i1 %cmp.not, %tobool.not
  br i1 %or.cond, label %if.end, label %cleanup

if.end:                                           ; preds = %entry
  %sub = sub nsw i32 %w, %l
  %cond = tail call i32 @llvm.smin.i32(i32 %sub, i32 256)
  %conv = sext i32 %cond to i64
  %conv2 = zext nneg i8 %c to i32
  %call = call ptr @memset(ptr noundef nonnull %buf, i32 noundef %conv2, i64 noundef %conv) #12
  %cmp331 = icmp sgt i32 %sub, 255
  br i1 %cmp331, label %for.body.lr.ph, label %for.end

for.body.lr.ph:                                   ; preds = %if.end
  %len.i = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  %cap.i = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 1
  br label %for.body

for.body:                                         ; preds = %emit.exit, %for.body.lr.ph
  %l.addr.032 = phi i32 [ %sub, %for.body.lr.ph ], [ %sub6, %emit.exit ]
  %0 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i = icmp eq ptr %0, null
  br i1 %tobool1.not.i, label %if.else.i, label %if.then2.i

if.then2.i:                                       ; preds = %for.body
  %1 = load i64, ptr %len.i, align 8, !tbaa !38
  %2 = load i64, ptr %cap.i, align 8, !tbaa !37
  %cmp.i = icmp ult i64 %1, %2
  br i1 %cmp.i, label %if.then3.i, label %emit.exit

if.then3.i:                                       ; preds = %if.then2.i
  %3 = xor i64 %1, -1
  %sub6.i = add i64 %2, %3
  %cond.i = call i64 @llvm.umin.i64(i64 %sub6.i, i64 256)
  %add.ptr.i = getelementptr inbounds i8, ptr %0, i64 %1
  %call.i = call ptr @memcpy(ptr noundef nonnull %add.ptr.i, ptr noundef nonnull %buf, i64 noundef %cond.i) #12
  br label %emit.exit

if.else.i:                                        ; preds = %for.body
  call void @__msil_write(ptr noundef nonnull %buf, i64 noundef 256) #12
  br label %emit.exit

emit.exit:                                        ; preds = %if.else.i, %if.then3.i, %if.then2.i
  %4 = load i64, ptr %len.i, align 8, !tbaa !38
  %add.i = add i64 %4, 256
  store i64 %add.i, ptr %len.i, align 8, !tbaa !38
  %sub6 = add nsw i32 %l.addr.032, -256
  %cmp3 = icmp sgt i32 %l.addr.032, 511
  br i1 %cmp3, label %for.body, label %for.end, !llvm.loop !79

for.end:                                          ; preds = %emit.exit, %if.end
  %l.addr.0.lcssa = phi i32 [ %sub, %if.end ], [ %sub6, %emit.exit ]
  %conv7 = sext i32 %l.addr.0.lcssa to i64
  %tobool.not.i = icmp eq i32 %l.addr.0.lcssa, 0
  br i1 %tobool.not.i, label %cleanup, label %if.end.i

if.end.i:                                         ; preds = %for.end
  %5 = load ptr, ptr %o, align 8, !tbaa !35
  %tobool1.not.i17 = icmp eq ptr %5, null
  br i1 %tobool1.not.i17, label %if.else.i29, label %if.then2.i18

if.then2.i18:                                     ; preds = %if.end.i
  %len.i19 = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  %6 = load i64, ptr %len.i19, align 8, !tbaa !38
  %cap.i20 = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 1
  %7 = load i64, ptr %cap.i20, align 8, !tbaa !37
  %cmp.i21 = icmp ult i64 %6, %7
  br i1 %cmp.i21, label %if.then3.i24, label %if.end11.i

if.then3.i24:                                     ; preds = %if.then2.i18
  %8 = xor i64 %6, -1
  %sub6.i25 = add i64 %7, %8
  %cond.i26 = call i64 @llvm.umin.i64(i64 %sub6.i25, i64 %conv7)
  %add.ptr.i27 = getelementptr inbounds i8, ptr %5, i64 %6
  %call.i28 = call ptr @memcpy(ptr noundef nonnull %add.ptr.i27, ptr noundef nonnull %buf, i64 noundef %cond.i26) #12
  br label %if.end11.i

if.else.i29:                                      ; preds = %if.end.i
  call void @__msil_write(ptr noundef nonnull %buf, i64 noundef %conv7) #12
  br label %if.end11.i

if.end11.i:                                       ; preds = %if.else.i29, %if.then3.i24, %if.then2.i18
  %len12.i22 = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  %9 = load i64, ptr %len12.i22, align 8, !tbaa !38
  %add.i23 = add i64 %9, %conv7
  store i64 %add.i23, ptr %len12.i22, align 8, !tbaa !38
  br label %cleanup

cleanup:                                          ; preds = %if.end11.i, %for.end, %entry
  call void @llvm.lifetime.end.p0(i64 256, ptr nonnull %buf) #13
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smin.i32(i32, i32) #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smin.i64(i64, i64) #7

declare dso_local ptr @memset(ptr noundef, i32 noundef, i64 noundef) local_unnamed_addr #3

; Function Attrs: nounwind
define dso_local i32 @vsprintf(ptr noundef %dst, ptr noundef %fmt, ptr noundef %ap) local_unnamed_addr #2 {
entry:
  %ap.addr.i = alloca ptr, align 8
  %o.i = alloca %struct.out, align 8
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap.addr.i)
  store ptr %ap, ptr %ap.addr.i, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %o.i) #13
  store ptr %dst, ptr %o.i, align 8, !tbaa !35
  %cap.i = getelementptr inbounds %struct.out, ptr %o.i, i64 0, i32 1
  store i64 -1, ptr %cap.i, align 8, !tbaa !37
  %len.i = getelementptr inbounds %struct.out, ptr %o.i, i64 0, i32 2
  store i64 0, ptr %len.i, align 8, !tbaa !38
  %call.i = call fastcc i32 @finish(ptr noundef nonnull %o.i, ptr noundef %fmt, ptr noundef nonnull %ap.addr.i) #14
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %o.i) #13
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap.addr.i)
  ret i32 %call.i
}

; Function Attrs: nounwind
define dso_local i32 @vprintf(ptr noundef %fmt, ptr noundef %ap) local_unnamed_addr #2 {
entry:
  %ap.addr = alloca ptr, align 8
  %o = alloca %struct.out, align 8
  store ptr %ap, ptr %ap.addr, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %o) #13
  store ptr null, ptr %o, align 8, !tbaa !35
  %cap = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 1
  store i64 0, ptr %cap, align 8, !tbaa !37
  %len = getelementptr inbounds %struct.out, ptr %o, i64 0, i32 2
  store i64 0, ptr %len, align 8, !tbaa !38
  %call = call fastcc i32 @finish(ptr noundef nonnull %o, ptr noundef %fmt, ptr noundef nonnull %ap.addr) #14
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %o) #13
  ret i32 %call
}

; Function Attrs: nounwind
define dso_local i32 @snprintf(ptr noundef %dst, i64 noundef %n, ptr noundef %fmt, ...) local_unnamed_addr #2 {
entry:
  %ap.addr.i = alloca ptr, align 8
  %o.i = alloca %struct.out, align 8
  %ap = alloca ptr, align 8
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap) #13
  call void @llvm.va_start(ptr nonnull %ap)
  %0 = load ptr, ptr %ap, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap.addr.i)
  store ptr %0, ptr %ap.addr.i, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %o.i) #13
  store ptr %dst, ptr %o.i, align 8, !tbaa !35
  %cap.i = getelementptr inbounds %struct.out, ptr %o.i, i64 0, i32 1
  store i64 %n, ptr %cap.i, align 8, !tbaa !37
  %len.i = getelementptr inbounds %struct.out, ptr %o.i, i64 0, i32 2
  store i64 0, ptr %len.i, align 8, !tbaa !38
  %call.i = call fastcc i32 @finish(ptr noundef nonnull %o.i, ptr noundef %fmt, ptr noundef nonnull %ap.addr.i) #14
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %o.i) #13
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap.addr.i)
  call void @llvm.va_end(ptr nonnull %ap)
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap) #13
  ret i32 %call.i
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start(ptr) #11

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end(ptr) #11

; Function Attrs: nounwind
define dso_local i32 @sprintf(ptr noundef %dst, ptr noundef %fmt, ...) local_unnamed_addr #2 {
entry:
  %ap.addr.i.i = alloca ptr, align 8
  %o.i.i = alloca %struct.out, align 8
  %ap = alloca ptr, align 8
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap) #13
  call void @llvm.va_start(ptr nonnull %ap)
  %0 = load ptr, ptr %ap, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap.addr.i.i)
  store ptr %0, ptr %ap.addr.i.i, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %o.i.i) #13
  store ptr %dst, ptr %o.i.i, align 8, !tbaa !35
  %cap.i.i = getelementptr inbounds %struct.out, ptr %o.i.i, i64 0, i32 1
  store i64 -1, ptr %cap.i.i, align 8, !tbaa !37
  %len.i.i = getelementptr inbounds %struct.out, ptr %o.i.i, i64 0, i32 2
  store i64 0, ptr %len.i.i, align 8, !tbaa !38
  %call.i.i = call fastcc i32 @finish(ptr noundef nonnull %o.i.i, ptr noundef %fmt, ptr noundef nonnull %ap.addr.i.i) #14
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %o.i.i) #13
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap.addr.i.i)
  call void @llvm.va_end(ptr nonnull %ap)
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap) #13
  ret i32 %call.i.i
}

; Function Attrs: nounwind
define dso_local i32 @printf(ptr noundef %fmt, ...) local_unnamed_addr #2 {
entry:
  %ap.addr.i = alloca ptr, align 8
  %o.i = alloca %struct.out, align 8
  %ap = alloca ptr, align 8
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap) #13
  call void @llvm.va_start(ptr nonnull %ap)
  %0 = load ptr, ptr %ap, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %ap.addr.i)
  store ptr %0, ptr %ap.addr.i, align 8, !tbaa !18
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %o.i) #13
  store ptr null, ptr %o.i, align 8, !tbaa !35
  %cap.i = getelementptr inbounds %struct.out, ptr %o.i, i64 0, i32 1
  store i64 0, ptr %cap.i, align 8, !tbaa !37
  %len.i = getelementptr inbounds %struct.out, ptr %o.i, i64 0, i32 2
  store i64 0, ptr %len.i, align 8, !tbaa !38
  %call.i = call fastcc i32 @finish(ptr noundef nonnull %o.i, ptr noundef %fmt, ptr noundef nonnull %ap.addr.i) #14
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %o.i) #13
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap.addr.i)
  call void @llvm.va_end(ptr nonnull %ap)
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %ap) #13
  ret i32 %call.i
}

; Function Attrs: nounwind
define dso_local noundef i32 @puts(ptr noundef %s) local_unnamed_addr #2 {
entry:
  %call = tail call i64 @strlen(ptr noundef %s) #12
  tail call void @__msil_write(ptr noundef %s, i64 noundef %call) #12
  tail call void @__msil_write(ptr noundef nonnull @"??_C@_01EEMJAFIK@?6?$AA@", i64 noundef 1) #12
  ret i32 0
}

; Function Attrs: nounwind
define dso_local noundef i32 @putchar(i32 noundef returned %c) local_unnamed_addr #2 {
entry:
  %ch = alloca i8, align 1
  call void @llvm.lifetime.start.p0(i64 1, ptr nonnull %ch) #13
  %conv = trunc i32 %c to i8
  store i8 %conv, ptr %ch, align 1, !tbaa !4
  call void @__msil_write(ptr noundef nonnull %ch, i64 noundef 1) #12
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %ch) #13
  ret i32 %c
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree norecurse nosync nounwind memory(read, inaccessiblemem: none) "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #5 = { nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #6 = { nofree norecurse nosync nounwind memory(argmem: read) "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #8 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #9 = { mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, argmem: none, inaccessiblemem: none) "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #10 = { mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none) "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #11 = { nocallback nofree nosync nounwind willreturn }
attributes #12 = { nobuiltin nounwind "no-builtins" }
attributes #13 = { nounwind }
attributes #14 = { nobuiltin "no-builtins" }

!llvm.ident = !{!0, !0, !0, !0}
!llvm.module.flags = !{!1, !2, !3}

!0 = !{!"clang version 18.1.7 (https://github.com/uwx/llvm-custom-builds bf36654b646b9ddee8ea6861acdd0af69edc6ab5)"}
!1 = !{i32 1, !"wchar_size", i32 2}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 1, !"MaxTLSAlign", i32 65536}
!4 = !{!5, !5, i64 0}
!5 = !{!"omnipotent char", !6, i64 0}
!6 = !{!"Simple C/C++ TBAA"}
!7 = distinct !{!7, !8}
!8 = !{!"llvm.loop.mustprogress"}
!9 = distinct !{!9, !8}
!10 = distinct !{!10, !8}
!11 = !{!12, !12, i64 0}
!12 = !{!"long long", !5, i64 0}
!13 = distinct !{!13, !8}
!14 = distinct !{!14, !8}
!15 = distinct !{!15, !8}
!16 = distinct !{!16, !8}
!17 = distinct !{!17, !8}
!18 = !{!19, !19, i64 0}
!19 = !{!"any pointer", !5, i64 0}
!20 = distinct !{!20, !8}
!21 = distinct !{!21, !8}
!22 = distinct !{!22, !8}
!23 = distinct !{!23, !8}
!24 = !{!25, !25, i64 0}
!25 = !{!"int", !5, i64 0}
!26 = distinct !{!26, !8}
!27 = distinct !{!27, !8}
!28 = distinct !{!28, !8}
!29 = !{!30, !12, i64 0}
!30 = !{!"", !12, i64 0, !12, i64 8}
!31 = !{!30, !12, i64 8}
!32 = distinct !{!32, !8}
!33 = distinct !{!33, !8}
!34 = distinct !{!34, !8}
!35 = !{!36, !19, i64 0}
!36 = !{!"out", !19, i64 0, !12, i64 8, !12, i64 16}
!37 = !{!36, !12, i64 8}
!38 = !{!36, !12, i64 16}
!39 = distinct !{!39, !8}
!40 = distinct !{!40, !8}
!41 = distinct !{!41, !8}
!42 = distinct !{!42, !8}
!43 = distinct !{!43, !8}
!44 = !{!45, !45, i64 0}
!45 = !{!"long", !5, i64 0}
!46 = !{!47, !47, i64 0}
!47 = !{!"short", !5, i64 0}
!48 = distinct !{!48, !8}
!49 = distinct !{!49, !8}
!50 = distinct !{!50, !8}
!51 = distinct !{!51, !8}
!52 = distinct !{!52, !8}
!53 = distinct !{!53, !8}
!54 = distinct !{!54, !8}
!55 = distinct !{!55, !8}
!56 = distinct !{!56, !8}
!57 = distinct !{!57, !8}
!58 = distinct !{!58, !8}
!59 = distinct !{!59, !8}
!60 = distinct !{!60, !8}
!61 = distinct !{!61, !62}
!62 = !{!"llvm.loop.unroll.disable"}
!63 = distinct !{!63, !8}
!64 = distinct !{!64, !8}
!65 = distinct !{!65, !8}
!66 = distinct !{!66, !8}
!67 = distinct !{!67, !8}
!68 = distinct !{!68, !62}
!69 = distinct !{!69, !8}
!70 = distinct !{!70, !62}
!71 = distinct !{!71, !8}
!72 = distinct !{!72, !8}
!73 = distinct !{!73, !62}
!74 = distinct !{!74, !8}
!75 = distinct !{!75, !8}
!76 = distinct !{!76, !62}
!77 = distinct !{!77, !8}
!78 = distinct !{!78, !8}
!79 = distinct !{!79, !8}
